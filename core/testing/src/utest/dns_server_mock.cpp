#include <userver/utest/dns_server_mock.hpp>

#include <netinet/in.h>

#include <cstdint>
#include <cstdlib>
#include <vector>

#include <fmt/format.h>

#include <userver/engine/async.hpp>
#include <userver/engine/io/sockaddr.hpp>
#include <userver/engine/task/cancel.hpp>
#include <userver/utest/utest.hpp>
#include <userver/utils/assert.hpp>

#include <userver/internal/net/net_listener.hpp>

USERVER_NAMESPACE_BEGIN

namespace utest {
namespace {

constexpr size_t kMaxMessageSize = 16 * 1024;

// For details on DNS message format see RFC1035
// https://datatracker.ietf.org/doc/html/rfc1035

constexpr uint16_t kResponseFlag = 0x8000;
constexpr uint16_t kOpcodeMask = 0x7800;
constexpr uint16_t kServFailCode = 0x0002;

constexpr uint16_t kInClass = 0x0001;

constexpr uint16_t kNameBackref =
    0xC00C;  // always the same for single question queries

class DnsMessageWriter {
 public:
  explicit DnsMessageWriter(char* buffer, size_t size)
      : buffer_{buffer}, size_{size}, pos_{buffer_} {}

  size_t CurrentPosition() const { return pos_ - buffer_; }

  void Skip(size_t offset) {
    CheckSpace(offset);
    pos_ += offset;
  }

  void SetCname(std::string cname) {
    UASSERT_MSG(cname_.empty(), "Only one CNAME may be present");
    UASSERT_MSG(!cname.empty(), "CNAME cannot be empty");
    cname_ = std::move(cname);
  }

  DnsMessageWriter& operator<<(uint16_t value) {
    CheckSpace(2);
    Push(static_cast<char>(value >> 8));
    Push(static_cast<char>(value));
    return *this;
  }

  DnsMessageWriter& operator<<(int32_t value) {
    CheckSpace(4);
    const auto shift_safe = static_cast<uint32_t>(value);
    Push(static_cast<char>(shift_safe >> 24));
    Push(static_cast<char>(shift_safe >> 16));
    Push(static_cast<char>(shift_safe >> 8));
    Push(static_cast<char>(shift_safe));
    return *this;
  }

  DnsMessageWriter& operator<<(const std::string& value) {
    UASSERT_MSG(value.size() + 2 < 256, "Too long domain name");
    CheckSpace(value.size() + 2);
    char* size_pos = pos_;
    Push(0);
    char current_label_size = 0;
    for (char c : value) {
      if (c == '.') {
        *size_pos = current_label_size;
        size_pos = pos_;
        Push(0);
        current_label_size = 0;
      } else {
        Push(c);
        ++current_label_size;
        UASSERT_MSG(current_label_size < 64, "Too long domain name part");
      }
    }
    *size_pos = current_label_size;
    Push(0);
    return *this;
  }

  DnsMessageWriter& operator<<(const DnsServerMock::DnsAnswer& answer) {
    if (cname_.empty() || answer.type == DnsServerMock::RecordType::kCname) {
      *this << kNameBackref;
    } else {
      *this << cname_;
    }
    *this << static_cast<uint16_t>(answer.type) << kInClass << answer.ttl;

    switch (answer.type) {
      case DnsServerMock::RecordType::kA: {
        UASSERT(std::holds_alternative<engine::io::Sockaddr>(answer.data));
        const auto* sa =
            std::get<engine::io::Sockaddr>(answer.data).As<sockaddr_in>();
        const uint16_t rdlength = sizeof(sa->sin_addr);
        *this << rdlength;
        CheckSpace(rdlength);
        std::memcpy(pos_, &sa->sin_addr, rdlength);
        pos_ += rdlength;
      } break;
      case DnsServerMock::RecordType::kAAAA: {
        UASSERT(std::holds_alternative<engine::io::Sockaddr>(answer.data));
        const auto* sa =
            std::get<engine::io::Sockaddr>(answer.data).As<sockaddr_in6>();
        const uint16_t rdlength = sizeof(sa->sin6_addr);
        *this << rdlength;
        CheckSpace(rdlength);
        std::memcpy(pos_, &sa->sin6_addr, rdlength);
        pos_ += rdlength;
      } break;
      case DnsServerMock::RecordType::kCname: {
        UASSERT(std::holds_alternative<std::string>(answer.data));
        const auto& alias = std::get<std::string>(answer.data);
        const uint16_t rdlength = alias.size() + 2;
        *this << rdlength << alias;
      } break;
      default:
        UASSERT_MSG(false, "Invalid answer type");
    }
    return *this;
  }

 private:
  void CheckSpace(size_t size) const {
    UASSERT_MSG(pos_ - buffer_ + size <= size_, "Insufficient buffer size");
  }

  void Push(char c) {
    *pos_ = c;
    ++pos_;
  }

  char* buffer_;
  const size_t size_;
  char* pos_;
  std::string cname_;
};

uint16_t GetNetworkUint16(const char* data) {
  return (static_cast<uint16_t>(data[0]) << 8) | static_cast<uint16_t>(data[1]);
}

auto ParseMessage(const char* data, size_t size) noexcept {
  DnsServerMock::DnsQuery query;
  UASSERT_MSG(size >= 12, "Truncated query");

  const auto flags = GetNetworkUint16(data + 2);
  UASSERT_MSG(!(flags & kResponseFlag), "Server received a response");
  UASSERT_MSG(!(flags & kOpcodeMask), "Only standard queries are accepted");

  const auto queries_num = GetNetworkUint16(data + 4);
  UASSERT_MSG(queries_num == 1, "Only single queries are supported");

  const char* pos = data + 12;
  while (*pos && static_cast<size_t>(pos - data) < size) {
    if (!query.name.empty()) query.name += '.';

    const char len = *pos;
    ++pos;
    query.name.append(pos, len);
    pos += len;
  }
  ++pos;

  UASSERT_MSG(static_cast<size_t>(pos - data + 4) <= size, "Truncated query");

  query.type = static_cast<DnsServerMock::RecordType>(GetNetworkUint16(pos));
  pos += 2;
  UASSERT_MSG(query.type == DnsServerMock::RecordType::kA ||
                  query.type == DnsServerMock::RecordType::kAAAA,
              "Only A and AAAA queries are supported");

  auto qclass = GetNetworkUint16(pos);
  UASSERT_MSG(qclass == kInClass, "Only IN queries are supported");

  return query;
}

// noexcept: should fail hard on logic errors
size_t UpdateForAnswer(const DnsServerMock::DnsAnswerVector& answers,
                       char* data, size_t original_size,
                       size_t buffer_size) noexcept {
  DnsMessageWriter writer{data, buffer_size};

  for (const auto& answer : answers) {
    if (answer.type == DnsServerMock::RecordType::kCname) {
      UASSERT(std::holds_alternative<std::string>(answer.data));
      writer.SetCname(std::get<std::string>(answer.data));
    }
  }

  writer.Skip(2);  // txn id
  writer << static_cast<uint16_t>(GetNetworkUint16(data + 2) | kResponseFlag);
  writer.Skip(2);  // queries count
  writer << static_cast<uint16_t>(answers.size());
  writer.Skip(original_size - 8);

  for (const auto& answer : answers) {
    writer << answer;
  }
  return writer.CurrentPosition();
}

size_t UpdateForServFail(char* data, size_t original_size) noexcept {
  DnsMessageWriter writer{data, original_size};
  writer.Skip(2);  // txn id
  writer << static_cast<uint16_t>(GetNetworkUint16(data + 2) | kResponseFlag |
                                  kServFailCode);
  return original_size;
}

}  // namespace

DnsServerMock::DnsServerMock(DnsHandler handler)
    // NOLINTNEXTLINE(cppcoreguidelines-slicing)
    : listener_(std::make_shared<internal::net::UdpListener>()),
      handler_{std::move(handler)},
      receiver_task_{engine::AsyncNoSpan([this] { ProcessRequests(); })} {}

std::string DnsServerMock::GetServerAddress() const {
  return fmt::to_string(listener_->addr);
}

void DnsServerMock::ProcessRequests() {
  // Too big for stack, keep it in dynamic memory
  std::vector<char> buffer{};
  buffer.resize(kMaxMessageSize);

  while (!engine::current_task::ShouldCancel()) {
    const auto iter_deadline =
        engine::Deadline::FromDuration(utest::kMaxTestWaitTime);
    const auto recv_result = listener_->socket.RecvSomeFrom(
        buffer.data(), buffer.size(), iter_deadline);
    size_t response_size = 0;
    try {
      const auto queries =
          ParseMessage(buffer.data(), recv_result.bytes_received);
      const auto answer = handler_(queries);
      response_size = UpdateForAnswer(
          answer, buffer.data(), recv_result.bytes_received, buffer.size());
    } catch (const NoAnswer&) {
      return;
    } catch (const std::exception&) {
      response_size =
          UpdateForServFail(buffer.data(), recv_result.bytes_received);
    }
    UASSERT(response_size);
    const auto sent_bytes = listener_->socket.SendAllTo(
        recv_result.src_addr, buffer.data(), response_size, iter_deadline);
    UASSERT(sent_bytes == response_size);
  }
}

}  // namespace utest

USERVER_NAMESPACE_END
