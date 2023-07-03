#include "time_storage.hpp"

#include <fmt/compile.h>
#include <fmt/format.h>

#include <userver/logging/impl/tag_writer.hpp>
#include <userver/utils/algo.hpp>

USERVER_NAMESPACE_BEGIN

namespace {

const std::string kTimerSuffix = "_time";
constexpr auto kNsInMs = 1'000'000;

}  // namespace

namespace tracing::impl {

void TimeStorage::PushLap(const std::string& key, Duration value) {
  data_[key] += value;
}

TimeStorage::Duration TimeStorage::DurationTotal(const std::string& key) const {
  return utils::FindOrDefault(data_, key, Duration{0});
}

void TimeStorage::MergeInto(logging::impl::TagWriter writer) {
  for (const auto& [key, value] : data_) {
    const auto ns_count = value.count();
    writer.PutTag(key + kTimerSuffix,
                  fmt::format(FMT_COMPILE("{}.{:0>6}"), ns_count / kNsInMs,
                              ns_count % kNsInMs));
  }
}

}  // namespace tracing::impl

USERVER_NAMESPACE_END
