#pragma once

/// @file userver/tracing/scope_time.hpp
/// @brief @copybrief tracing::ScopeTime

#include <atomic>
#include <chrono>
#include <string>
#include <unordered_map>

#include <userver/logging/log_extra.hpp>

USERVER_NAMESPACE_BEGIN

namespace tracing {

namespace impl {

using PerfTimePoint = std::chrono::steady_clock::time_point;

class TimeStorage {
 public:
  using RealMilliseconds = std::chrono::duration<double, std::milli>;

 public:
  explicit TimeStorage(std::string name);

  TimeStorage(const TimeStorage&) = delete;
  TimeStorage(TimeStorage&&) noexcept = default;

  void PushLap(const std::string& key, double value);
  void PushLap(const std::string& key, RealMilliseconds value);

  /// Elapsed time since TimeStorage object creation
  RealMilliseconds Elapsed() const;
  /// Accumulated time for a certain key. If the key is not there, returns 0
  RealMilliseconds ElapsedTotal(const std::string& key) const;

  enum class TotalTime {
    kWith,
    kWithout,
  };
  logging::LogExtra GetLogs(TotalTime total_time = TotalTime::kWith);

 private:
  const PerfTimePoint start_;
  const std::string name_;

  std::unordered_map<std::string, RealMilliseconds> data_;
};

}  // namespace impl

/// @brief Type to measure execution time of a scope
///
/// Use tracing::Span::CreateScopeTime() to construct
class ScopeTime {
 public:
  using Duration = impl::TimeStorage::RealMilliseconds;

  explicit ScopeTime(impl::TimeStorage& ts);
  ScopeTime(impl::TimeStorage& ts, std::string scope_name);
  ScopeTime(const ScopeTime&) = delete;
  ScopeTime(ScopeTime&&) = default;
  ~ScopeTime();

  /// Records the current scope time if the name is set, and stops the timer
  Duration Reset();

  /// Records the current scope time if the name is set, and starts a new one
  Duration Reset(std::string scope_name);

  /// Stops the timer without recording its value
  void Discard();

  /// Returns time elapsed since last reset
  /// Will return 0 if the timer is stopped
  Duration ElapsedSinceReset() const;

  /// Returns total time elapsed for a certain scope. If there is no record for
  /// the scope, returns 0
  Duration ElapsedTotal(const std::string& scope_name) const;

  /// Returns total time elapsed for current scope
  /// Will return 0 if the timer is stopped
  Duration ElapsedTotal() const;

  const std::string& CurrentScope() const { return scope_name_; }

 private:
  impl::TimeStorage& ts_;
  impl::PerfTimePoint start_;
  std::string scope_name_;
};

}  // namespace tracing

USERVER_NAMESPACE_END
