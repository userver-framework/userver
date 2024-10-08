#pragma once

/// @file userver/utils/statistics/busy.hpp
/// @brief @copybrief utils::statistics::BusyMarker

#include <memory>

#include <userver/utils/statistics/recentperiod.hpp>

USERVER_NAMESPACE_BEGIN

namespace utils::statistics {

/// Measure how much time we've spent in work recently in percents that
/// supports recirsive starts. Use
/// utils::statistics::BusyMarker for RAII time measures.
///
/// @snippet utils/statistics/busy_test.cpp  busy sample
class BusyStorage final {
 public:
  using Duration = std::chrono::steady_clock::duration;

  BusyStorage(Duration epoch_duration, Duration history_period);

  ~BusyStorage();

  /// Safe to read concurrently with calling StartWork() and StopWork()
  double GetCurrentLoad() const;

  /// Starts the time measure, if it was not already started
  void StartWork();

  /// Stops the time measure if the count of StopWork() invocations matches the
  /// StartWork() invocations count.
  void StopWork() noexcept;

  /// Returns true if the time measure is active
  bool IsAlreadyStarted() const noexcept;

 private:
  Duration GetNotCommittedLoad() const noexcept;

  struct Impl;
  std::unique_ptr<Impl> pimpl;
};

/// @brief A RAII-style guard to account code block execution time in
/// utils::statistics::BusyStorage. Aware of recursive invokations in the same
/// thread.
///
/// @snippet utils/statistics/busy_test.cpp  busy sample
class BusyMarker final {
 public:
  BusyMarker(BusyStorage& storage) : storage_(storage) { storage_.StartWork(); }

  BusyMarker(const BusyMarker&) = delete;
  BusyMarker& operator=(const BusyMarker&) = delete;

  ~BusyMarker() { storage_.StopWork(); }

 private:
  BusyStorage& storage_;
};

}  // namespace utils::statistics

USERVER_NAMESPACE_END
