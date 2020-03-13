#include <utils/prof.hpp>

#include <boost/format.hpp>

#include <logging/log.hpp>
#include <utils/assert.hpp>

namespace {
const std::string kStopWatchAttrName = "stopwatch_name";
const std::string kTotalTimeAttrName = "total_time";
const std::string kTimeUnitsAttrName = "stopwatch_units";
const std::string kStartTimestampAttrName = "start_timestamp";
}  // namespace

using real_milliseconds_t = TimeStorage::RealMilliseconds;

SwLogger::SwLogger(TimeStorage& ts, const logging::LogExtra& log_extra)
    : ts_(ts), log_extra_(log_extra) {}

SwLogger::~SwLogger() { LOG_INFO() << ts_.GetLogs() << log_extra_; }

TimeStorage::TimeStorage(const std::string& name)
    : n_children_(0),
      parent_(nullptr),
      start_(PerfTimePoint::clock::now()),
      name_(name) {}

TimeStorage::TimeStorage(TimeStorage* parent)
    : n_children_(0), parent_(parent) {}

TimeStorage::~TimeStorage() {
  UASSERT(n_children_.load(std::memory_order_consume) == 0);
  if (parent_ != nullptr) parent_->MergeBack(std::move(*this));
}

TimeStorage TimeStorage::CreateChild() {
  TimeStorage* const root = parent_ != nullptr ? parent_ : this;
  root->n_children_.fetch_add(1, std::memory_order_relaxed);
  return TimeStorage(root);
}

void TimeStorage::PushLap(const std::string& key, double value) {
  data_[key] += value;
}

TimeStorage::RealMilliseconds TimeStorage::Elapsed() const {
  return std::chrono::duration_cast<real_milliseconds_t>(
      PerfTimePoint::clock::now() - start_);
}

logging::LogExtra TimeStorage::GetLogs() {
  const auto duration = Elapsed();

  std::unique_lock<std::mutex> lock(m_);
  UASSERT(n_children_.load(std::memory_order_consume) == 0);
  for (const auto& child : children_) {
    for (const auto& kv : child) {
      data_[kv.first] += kv.second;
    }
  }
  children_.clear();

  const double start_ts = start_.time_since_epoch().count() / 1000000000.0;
  const auto start_ts_str = str(boost::format("%.6f") % start_ts);

  logging::LogExtra result({{kStopWatchAttrName, name_},
                            {kTimeUnitsAttrName, "ms"},
                            {kTotalTimeAttrName, duration.count()},
                            {kStartTimestampAttrName, start_ts_str}});
  result.ExtendRange(data_.cbegin(), data_.cend());

  return result;
}

void TimeStorage::MergeBack(TimeStorage&& child) {
  std::unique_lock<std::mutex> lock(m_);
  children_.emplace_back(std::move(child.data_));
  n_children_.fetch_sub(1, std::memory_order_release);
}

TimeStorage::TimeStorage(TimeStorage&& other) noexcept
    : data_(std::move(other.data_)), n_children_(0), parent_(other.parent_) {
  other.parent_ = nullptr;
  UASSERT(other.n_children_.load(std::memory_order_relaxed) == 0);
}

const logging::LogExtra LoggingTimeStorage::kEmptyLogExtra;

ScopeTime::ScopeTime(TimeStorage& ts) : ts_(ts) {}

ScopeTime::ScopeTime(TimeStorage& ts, const std::string& scope_name)
    : ScopeTime(ts) {
  Reset(scope_name);
}

ScopeTime::~ScopeTime() { Reset(); }

TimeStorage::RealMilliseconds ScopeTime::Reset() {
  if (scope_name_.empty()) return TimeStorage::RealMilliseconds(0);

  const auto end = PerfTimePoint::clock::now();
  const auto duration =
      std::chrono::duration_cast<real_milliseconds_t>(end - start_);
  ts_.PushLap(scope_name_, duration.count());
  scope_name_.clear();
  return duration;
}

TimeStorage::RealMilliseconds ScopeTime::Reset(const std::string& scope_name) {
  auto result = Reset();
  scope_name_ = scope_name + "_time";
  start_ = PerfTimePoint::clock::now();
  return result;
}

void ScopeTime::Discard() { scope_name_.clear(); }
