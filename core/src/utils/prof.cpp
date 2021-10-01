#include <userver/utils/prof.hpp>

#include <fmt/compile.h>
#include <fmt/format.h>

#include <userver/logging/log.hpp>
#include <userver/utils/algo.hpp>
#include <userver/utils/assert.hpp>

namespace {
const std::string kStopWatchAttrName = "stopwatch_name";
const std::string kTotalTimeAttrName = "total_time";
const std::string kTimeUnitsAttrName = "stopwatch_units";
const std::string kStartTimestampAttrName = "start_timestamp";

const std::string kTimerSuffix = "_time";

using RealSeconds = std::chrono::duration<double>;
}  // namespace

SwLogger::SwLogger(TimeStorage& ts, const logging::LogExtra& log_extra)
    : ts_(ts), log_extra_(log_extra) {}

SwLogger::~SwLogger() { LOG_INFO() << ts_.GetLogs() << log_extra_; }

TimeStorage::TimeStorage(std::string name)
    : start_(PerfTimePoint::clock::now()), name_(std::move(name)) {}

void TimeStorage::PushLap(const std::string& key, double value) {
  PushLap(key, RealMilliseconds{value});
}

void TimeStorage::PushLap(const std::string& key, RealMilliseconds value) {
  data_[key] += value;
}

TimeStorage::RealMilliseconds TimeStorage::Elapsed() const {
  return PerfTimePoint::clock::now() - start_;
}

TimeStorage::RealMilliseconds TimeStorage::ElapsedTotal(
    const std::string& key) const {
  return utils::FindOrDefault(data_, key, RealMilliseconds{0});
}

logging::LogExtra TimeStorage::GetLogs(TotalTime total_time) {
  const auto duration = Elapsed();
  const double start_ts =
      std::chrono::duration_cast<RealSeconds>(start_.time_since_epoch())
          .count();
  const auto start_ts_str = fmt::format(FMT_COMPILE("{:.6f}"), start_ts);

  logging::LogExtra result;
  if (total_time == TotalTime::kWith) {
    result.Extend(kStopWatchAttrName, name_);
    result.Extend(kTimeUnitsAttrName, "ms");
    result.Extend(kTotalTimeAttrName, duration.count());
    result.Extend(kStartTimestampAttrName, start_ts_str);
  }

  for (const auto& [key, value] : data_) {
    result.Extend(key + kTimerSuffix, value.count());
  }

  return result;
}

const logging::LogExtra LoggingTimeStorage::kEmptyLogExtra;

ScopeTime::ScopeTime(TimeStorage& ts) : ts_(ts) {}

ScopeTime::ScopeTime(TimeStorage& ts, std::string scope_name) : ScopeTime(ts) {
  Reset(std::move(scope_name));
}

ScopeTime::~ScopeTime() { Reset(); }

TimeStorage::RealMilliseconds ScopeTime::Reset() {
  if (scope_name_.empty()) return TimeStorage::RealMilliseconds(0);

  const TimeStorage::RealMilliseconds duration =
      PerfTimePoint::clock::now() - start_;
  ts_.PushLap(scope_name_, duration);
  scope_name_.clear();
  return duration;
}

TimeStorage::RealMilliseconds ScopeTime::Reset(std::string scope_name) {
  auto result = Reset();
  scope_name_ = std::move(scope_name);
  start_ = PerfTimePoint::clock::now();
  return result;
}

void ScopeTime::Discard() { scope_name_.clear(); }

TimeStorage::RealMilliseconds ScopeTime::ElapsedSinceReset() const {
  if (scope_name_.empty()) return TimeStorage::RealMilliseconds{0};
  return PerfTimePoint::clock::now() - start_;
}

TimeStorage::RealMilliseconds ScopeTime::ElapsedTotal(
    const std::string& scope_name) const {
  const auto duration = ts_.ElapsedTotal(scope_name);
  return scope_name_ == scope_name ? duration + ElapsedSinceReset() : duration;
}

TimeStorage::RealMilliseconds ScopeTime::ElapsedTotal() const {
  if (scope_name_.empty()) return TimeStorage::RealMilliseconds{0};
  return ts_.ElapsedTotal(scope_name_) + ElapsedSinceReset();
}
