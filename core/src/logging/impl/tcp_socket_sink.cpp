#include "tcp_socket_sink.hpp"

#include <arpa/inet.h>
#include <netdb.h>
#include <netinet/in.h>
#include <netinet/tcp.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <cstring>
#include <logging/log_helper_impl.hpp>

#include <spdlog/pattern_formatter.h>

USERVER_NAMESPACE_BEGIN

namespace logging::impl {

TcpSocketClient::TcpSocketClient(std::vector<engine::io::Sockaddr> addrs)
    : addrs_(std::move(addrs)) {}

void TcpSocketClient::Connect() {
  Close();
  for (const auto& addr : addrs_) {
    socket_ =
        engine::io::Socket{addr.Domain(), engine::io::SocketType::kStream};
    try {
      socket_.Connect(addr, {});
      break;
    } catch (std::exception&) {
      continue;
    }
  }
  if (socket_.Fd() == -1) {
    std::string list{};
    for (const auto& addr : addrs_) {
      list += addr.PrimaryAddressString() + ", ";
    }
    throw std::runtime_error(
        fmt::format("TcpSocketSink failed to connect to {}", list));
  }
  socket_.SetOption(IPPROTO_TCP, TCP_NODELAY, 1);
}

void TcpSocketClient::Send(const char* data, size_t n_bytes) {
  auto send_result = socket_.SendAll(data, n_bytes, {});
  if (n_bytes != send_result) {
    throw std::runtime_error(fmt::format(
        "Failed to send {} bytes because the remote closed the connection",
        n_bytes));
  }
}

void TcpSocketClient::Close() { socket_.Close(); }

bool TcpSocketClient::IsConnected() { return socket_.Fd() != -1; }

TcpSocketClient::~TcpSocketClient() { socket_.Close(); }

TcpSocketSink::TcpSocketSink(std::vector<engine::io::Sockaddr> addr)
    : client_{std::move(addr)},
      formatter_{std::make_unique<spdlog::pattern_formatter>()} {}

void TcpSocketSink::Close() {
  std::lock_guard lock{client_mutex_};
  client_.Close();
}

void TcpSocketSink::log(const spdlog::details::log_msg& msg) {
  spdlog::memory_buf_t formatted;
  formatter_->format(msg, formatted);

  std::lock_guard lock{client_mutex_};
  if (!client_.IsConnected()) {
    client_.Connect();
  }
  client_.Send(formatted.data(), formatted.size());
}

void TcpSocketSink::flush() {}

void TcpSocketSink::set_pattern(const std::string& pattern) {
  set_formatter(std::make_unique<spdlog::pattern_formatter>(pattern));
}

void TcpSocketSink::set_formatter(
    std::unique_ptr<spdlog::formatter> sink_formatter) {
  formatter_ = std::move(sink_formatter);
}

}  // namespace logging::impl

USERVER_NAMESPACE_END
