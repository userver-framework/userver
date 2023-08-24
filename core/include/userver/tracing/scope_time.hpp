#pragma once

/// @file userver/tracing/scope_time.hpp
/// @brief @copybrief tracing::ScopeTime

#include <atomic>
#include <chrono>
#include <string>

#include <userver/logging/log_extra.hpp>

USERVER_NAMESPACE_BEGIN

namespace tracing {

namespace impl {

class TimeStorage;

}  // namespace impl

/// @brief Type to measure execution time of a scope
///
/// Use tracing::Span::CreateScopeTime() to construct
class ScopeTime {
 public:
  using Duration = std::chrono::nanoseconds;
  using DurationMillis = std::chrono::duration<double, std::milli>;

  /// @brief Creates a tracing::ScopeTime attached to
  /// tracing::Span::CurrentSpan().
  ///
  /// Equivalent to tracing::Span::CurrentSpan().CreateScopeTime()
  ScopeTime();

  /// @brief Creates a tracing::ScopeTime attached to
  /// tracing::Span::CurrentSpan() and starts measuring execution time.
  ///
  /// Equivalent to tracing::Span::CurrentSpan().CreateScopeTime(scope_name)
  explicit ScopeTime(std::string scope_name);

  /// @cond
  // Constructors for internal use
  explicit ScopeTime(impl::TimeStorage& ts);
  ScopeTime(impl::TimeStorage& ts, std::string scope_name);
  /// @endcond

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
  Duration DurationSinceReset() const;

  /// Returns total time elapsed for a certain scope. If there is no record for
  /// the scope, returns 0
  Duration DurationTotal(const std::string& scope_name) const;

  /// Returns total time elapsed for current scope
  /// Will return 0 if the timer is stopped
  Duration DurationTotal() const;

  /// Returns time elapsed since last reset, returns 0 if the timer is stopped.
  ///
  /// Prefer using ScopeTime::DurationSinceReset()
  DurationMillis ElapsedSinceReset() const;

  /// Returns total time elapsed for a certain scope. If there is no record for
  /// the scope, returns 0.
  ///
  /// Prefer using ScopeTime::DurationTotal()
  DurationMillis ElapsedTotal(const std::string& scope_name) const;

  /// Returns total time elapsed for current scope
  /// Will return 0 if the timer is stopped.
  ///
  /// Prefer using ScopeTime::DurationTotal()
  DurationMillis ElapsedTotal() const;

  const std::string& CurrentScope() const { return scope_name_; }

 private:
  impl::TimeStorage& ts_;
  std::chrono::steady_clock::time_point start_;
  std::string scope_name_;
};

}  // namespace tracing

USERVER_NAMESPACE_END
