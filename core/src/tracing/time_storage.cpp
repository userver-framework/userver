#include "time_storage.hpp"

#include <userver/utils/algo.hpp>

USERVER_NAMESPACE_BEGIN

namespace {

const std::string kTimerSuffix = "_time";
constexpr auto kConvertToMilli = 0.000001;

}  // namespace

namespace tracing::impl {

void TimeStorage::PushLap(const std::string& key, Duration value) {
  data_[key] += value;
}

TimeStorage::Duration TimeStorage::DurationTotal(const std::string& key) const {
  return utils::FindOrDefault(data_, key, Duration{0});
}

void TimeStorage::MergeInto(logging::LogExtra& result) {
  for (const auto& [key, value] : data_) {
    result.Extend(key + kTimerSuffix, value.count() * kConvertToMilli);
  }
}

}  // namespace tracing::impl

USERVER_NAMESPACE_END
