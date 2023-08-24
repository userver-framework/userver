#pragma once

#include <memory>

#include <userver/utils/statistics/recentperiod.hpp>

USERVER_NAMESPACE_BEGIN

namespace utils::statistics {

using Duration = std::chrono::steady_clock::duration;

/* Storage of how much time we've spent in work recently, in %.
 * 'max_workers' limits number of threads working at the same time. */
class BusyStorage final {
 public:
  BusyStorage(Duration epoch_duration, Duration history_period,
              size_t max_workers = 1);

  ~BusyStorage();

  double GetCurrentLoad() const;

  using WorkerId = size_t;

  WorkerId StartWork();

  void StopWork(WorkerId worker_id);

 private:
  WorkerId PopWorkerId();

  Duration GetNotCommittedLoad(WorkerId worker_id) const;

  bool IsAlreadyStarted() const;

  bool UpdateCurrentWorkerLoad(std::vector<Duration>& load) const;

  struct Impl;
  std::unique_ptr<Impl> pimpl;

  friend class BusyMarker;
};

/* A RAII-style guard to account code block execution time in BusyStorage.
 * It is reentrant, IOW you may create BusyMarker inside of another BusyMarker
 * and work time is accounted correctly (based on the outer BusyMarker
 * lifetime).
 */
class BusyMarker final {
 public:
  BusyMarker(BusyStorage& storage)
      : storage_(storage), worker_id_(storage.StartWork()) {}

  BusyMarker(const BusyMarker&) = delete;
  BusyMarker& operator=(const BusyMarker&) = delete;

  ~BusyMarker() { storage_.StopWork(worker_id_); }

 private:
  BusyStorage& storage_;
  BusyStorage::WorkerId worker_id_;
};

}  // namespace utils::statistics

USERVER_NAMESPACE_END
