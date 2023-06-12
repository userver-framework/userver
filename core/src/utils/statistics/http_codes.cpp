#include <utils/statistics/http_codes.hpp>

#include <fmt/format.h>

#include <userver/formats/json/value_builder.hpp>
#include <userver/logging/log.hpp>
#include <userver/utils/assert.hpp>
#include <userver/utils/enumerate.hpp>
#include <userver/utils/statistics/writer.hpp>

USERVER_NAMESPACE_BEGIN

namespace utils::statistics {
namespace {

bool IsForcedStatusCode(HttpCodes::Code status) noexcept {
  return status == 200 || status == 400 || status == 401 || status == 500;
}

}  // namespace

HttpCodes::HttpCodes() {
  // TODO remove in C++20 after atomics value-initialization
  for (auto& counter : codes_) {
    counter.store(0, std::memory_order_relaxed);
  }
}

void HttpCodes::Account(Code code) noexcept {
  if (code < kMinHttpStatus || code >= kMaxHttpStatus) {
    LOG_ERROR() << "Invalid HTTP code encountered: " << code
                << ", skipping statistics accounting";
    return;
  }
  codes_[code - kMinHttpStatus].fetch_add(1, std::memory_order_relaxed);
}

HttpCodes::Snapshot::Snapshot(const HttpCodes& other) noexcept {
  for (std::size_t i = 0; i < codes_.size(); ++i) {
    codes_[i] = other.codes_[i].load(std::memory_order_relaxed);
  }
}

void HttpCodes::Snapshot::operator+=(const Snapshot& other) {
  for (std::size_t i = 0; i < codes_.size(); ++i) {
    codes_[i] += other.codes_[i];
  }
}

void DumpMetric(Writer& writer, const HttpCodes::Snapshot& snapshot) {
  for (const auto& [base_code, count] : utils::enumerate(snapshot.codes_)) {
    if (count != 0 || IsForcedStatusCode(base_code)) {
      const auto code = base_code + HttpCodes::kMinHttpStatus;
      writer.ValueWithLabels(count, {"http_code", std::to_string(code)});
    }
  }
}

static_assert(kHasWriterSupport<HttpCodes::Snapshot>);

}  // namespace utils::statistics

USERVER_NAMESPACE_END
