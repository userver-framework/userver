#include <utils/statistics/http_codes.hpp>

#include <fmt/format.h>

#include <userver/formats/json/value_builder.hpp>
#include <userver/logging/log.hpp>
#include <userver/utils/assert.hpp>
#include <userver/utils/enumerate.hpp>
#include <userver/utils/statistics/metadata.hpp>

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

HttpCodes::Snapshot HttpCodes::GetSnapshot() const {
  Snapshot result;
  for (const auto& [code, counter] : utils::enumerate(codes_)) {
    const auto count = counter.load(std::memory_order_relaxed);
    if (count != 0 || IsForcedStatusCode(code)) {
      result.codes[code + kMinHttpStatus] += count;
    }
  }
  return result;
}

void HttpCodes::Snapshot::Add(const Snapshot& other) {
  for (const auto& [code, count] : other.codes) {
    codes[code] += count;
  }
}

formats::json::Value Serialize(const HttpCodes::Snapshot& value,
                               formats::serialize::To<formats::json::Value>) {
  formats::json::ValueBuilder result(formats::common::Type::kObject);
  for (const auto& [code, count] : value.codes) {
    result[std::to_string(code)] = count;
  }
  utils::statistics::SolomonChildrenAreLabelValues(result, "http_code");
  return result.ExtractValue();
}

void DumpMetric(Writer writer, const HttpCodes::Snapshot& snapshot) {
  for (const auto& [code, count] : snapshot.codes) {
    writer.ValueWithLabels(count, {"http_code", std::to_string(code)});
  }
}

}  // namespace utils::statistics

USERVER_NAMESPACE_END
