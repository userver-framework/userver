#pragma once

#include <atomic>
#include <chrono>
#include <mutex>
#include <unordered_map>
#include <vector>

#include <logging/log_extra.hpp>

#define SCOPE_TIME_PREPARE(name, log_extra)          \
  TimeStorage prof_ts(name);                         \
  const SwLogger prof_sw_logger(prof_ts, log_extra); \
  ScopeTime prof_st_root(prof_ts)
#define SCOPE_TIME(name) \
  const ScopeTime BOOST_JOIN(prof_st_, __LINE__)(prof_ts, name)
#define SCOPE_TIME_FUNC(name, call) (ScopeTime(prof_ts, name), call)
#define SCOPE_TIME_BLOCK(name, call) \
  do {                               \
    SCOPE_TIME(name);                \
    call                             \
  } while (false)
#define SCOPE_TIME_RESET(name) prof_st_root.Reset(name)

using PerfTimePoint = std::chrono::high_resolution_clock::time_point;

class TimeStorage {
 public:
  using RealMilliseconds = std::chrono::duration<double, std::milli>;

 public:
  explicit TimeStorage(const std::string& name);
  TimeStorage(TimeStorage&&) noexcept;
  TimeStorage(const TimeStorage&) = delete;
  ~TimeStorage();

  TimeStorage CreateChild();
  void PushLap(const std::string& key, double value);

  /// Elapsed time since TimeStorage object creation
  RealMilliseconds Elapsed() const;
  /// Accumulated time for a certain key. If the key is not there, returns 0
  RealMilliseconds ElapsedTotal(const std::string& key) const;
  logging::LogExtra GetLogs();

 private:
  explicit TimeStorage(TimeStorage* parent);
  void MergeBack(TimeStorage&& child);

  using storage_t = std::unordered_map<std::string, double>;
  storage_t data_;
  std::vector<storage_t> children_;

  std::atomic_int n_children_;
  std::mutex m_;

  TimeStorage* parent_;
  const PerfTimePoint start_;
  const std::string name_;
};

class SwLogger {
 public:
  SwLogger(TimeStorage& ts, const logging::LogExtra& log_extra);
  ~SwLogger();

 private:
  TimeStorage& ts_;
  const logging::LogExtra& log_extra_;
};

class LoggingTimeStorage : public TimeStorage, protected SwLogger {
 public:
  explicit LoggingTimeStorage(
      const std::string& name,
      const logging::LogExtra& log_extra = kEmptyLogExtra)
      : TimeStorage(name), SwLogger(*this, log_extra) {}

 private:
  static const logging::LogExtra kEmptyLogExtra;
};

class ScopeTime {
 public:
  explicit ScopeTime(TimeStorage& ts);
  ScopeTime(TimeStorage& ts, const std::string& scope_name);
  ScopeTime(const ScopeTime&) = delete;
  ScopeTime(ScopeTime&&) = default;
  ~ScopeTime();

  /// Records the current scope time if the name is set, and stops the timer
  TimeStorage::RealMilliseconds Reset();

  /// Records the current scope time if the name is set, and starts a new one
  TimeStorage::RealMilliseconds Reset(const std::string& scope_name);

  /// Stops the timer without recording its value
  void Discard();

  /// Returns time elapsed since last reset
  /// Will return 0 if the timer is stopped
  TimeStorage::RealMilliseconds ElapsedSinceReset() const;

  /// Returns total time elapsed for a certain scope. If there is no record for
  /// the scope, returns 0
  TimeStorage::RealMilliseconds ElapsedTotal(
      const std::string& scope_name) const;

  /// Returns total time elapsed for current scope
  /// Will return 0 if the timer is stopped
  TimeStorage::RealMilliseconds ElapsedTotal() const;

  const std::string& CurrentScope() const { return scope_name_; }

 private:
  TimeStorage& ts_;
  PerfTimePoint start_;
  std::string scope_name_;
};
