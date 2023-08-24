#include <userver/utils/statistics/busy.hpp>

#include <boost/lockfree/queue.hpp>
#include <optional>
#include <vector>

#include <userver/logging/log.hpp>

#include <userver/utils/datetime.hpp>

USERVER_NAMESPACE_BEGIN

namespace utils::statistics {

using Timer = utils::datetime::SteadyClock;

struct BusyCounter {
  std::atomic<Duration> value{};

  void Reset() { value = Duration(0); }
};

class BusyResult {
 public:
  void Add(const BusyCounter& v, Duration this_epoch_duration,
           Duration before_this_epoch_duration) {
    const auto max_value = this_epoch_duration + before_this_epoch_duration;
    const auto value = std::min(v.value.load(), max_value);
    result_ += value;
    total_ = std::max(total_, max_value);
  }

  [[nodiscard]] Duration Get() const { return std::min(result_, total_); }
  [[nodiscard]] Duration Total() const { return total_; }

 private:
  Duration result_{};
  Duration total_{};
};

namespace {
constexpr BusyStorage::WorkerId kInvalidWorkerId{-1UL};
}

struct BusyStorage::Impl {
  Impl(Duration epoch_duration, Duration history_period, size_t max_workers);

  mutable RecentPeriod<BusyCounter, BusyResult, Timer> recent_period;
  std::vector<std::optional<Timer::time_point>> start_work{std::nullopt};
  boost::lockfree::queue<WorkerId> free_worker_ids;
};

BusyStorage::Impl::Impl(Duration epoch_duration, Duration history_period,
                        size_t max_workers)
    : recent_period(epoch_duration, history_period), free_worker_ids(1) {
  start_work.resize(max_workers);
  free_worker_ids.reserve(max_workers);

  for (size_t i = 0; i < max_workers; i++) free_worker_ids.push(i);
}

BusyStorage::BusyStorage(Duration epoch_duration, Duration history_period,
                         size_t max_workers)
    // FP(?): Possible leak in boost.lockfree
    // NOLINTNEXTLINE(clang-analyzer-cplusplus.NewDeleteLeaks)
    : pimpl(new Impl(epoch_duration, history_period, max_workers)) {}

BusyStorage::~BusyStorage() = default;

double BusyStorage::GetCurrentLoad() const {
  pimpl->recent_period.UpdateEpochIfOld();

  const auto max_workers = pimpl->start_work.size();
  std::vector<Duration> current_worker_load_duration(max_workers);

  for (size_t i = 0; i < max_workers; i++)
    current_worker_load_duration[i] = GetNotCommittedLoad(i);

  /*
   * StopWork() might change recent_period while we're reading recent_period and
   * start_work[]. We're OK if we read recent_period, start_work[] while nobody
   * does StopWork() committing large Duration (StartWork() doesn't
   * significantly affect the result, so it is OK to simply ignore).
   */
  while (true) {
    const auto result =
        pimpl->recent_period.GetStatsForPeriod(Timer::duration::min(), true);
    const auto load_duration = result.Get();
    auto duration = result.Total();
    if (duration.count() == 0) duration = pimpl->recent_period.GetMaxDuration();

    if (!UpdateCurrentWorkerLoad(current_worker_load_duration)) {
      auto final_load_duration = load_duration;
      for (const auto load_duration_it : current_worker_load_duration) {
        final_load_duration += std::min(load_duration_it, duration);
      }
      return static_cast<double>(final_load_duration.count()) /
             duration.count() / max_workers;
    }
  }
}

bool BusyStorage::UpdateCurrentWorkerLoad(std::vector<Duration>& load) const {
  const auto size = load.size();
  for (size_t i = 0; i < size; i++) {
    if (load[i] > GetNotCommittedLoad(i)) {
      /* WorkerId i has just committed its work, we have to re-read
       * recent_period */
      load[i] = Duration();
      return true;
    } else {
      /* Either nobody committed anything or committed only a very small
       * Duration which is less than (now() - time of the first
       * GetNotCommittedLoad(i)).
       */
    }
  }

  return false;
}

namespace {
thread_local std::vector<const BusyStorage*> this_thread_busy_storages;
}

bool BusyStorage::IsAlreadyStarted() const {
  return std::find(this_thread_busy_storages.begin(),
                   this_thread_busy_storages.end(),
                   this) != this_thread_busy_storages.end();
}

BusyStorage::WorkerId BusyStorage::StartWork() {
  auto worker_id = kInvalidWorkerId;
  if (!IsAlreadyStarted()) worker_id = PopWorkerId();
  if (worker_id != kInvalidWorkerId) {
    pimpl->start_work[worker_id] = utils::datetime::SteadyNow();
  }

  return worker_id;
}

void BusyStorage::StopWork(WorkerId worker_id) {
  if (worker_id == kInvalidWorkerId) return;

  auto not_committed_load = GetNotCommittedLoad(worker_id);
  auto& value = pimpl->recent_period.GetCurrentCounter().value;
  value = value.load() + not_committed_load;
  pimpl->start_work[worker_id] = std::nullopt;

  const auto* busy_storage_back = this_thread_busy_storages.back();
  if (busy_storage_back != this) {
    LOG_ERROR()
        << "StopWork() found wrong BusyStorage on this_thread's "
           "this_thread_busy_storages stack top! StartWork() and StopWork() "
           "was called from different threads, which is wrong. "
           "Current load of this BusyStorage is now broken and must not be "
           "trusted.";
  } else {
    this_thread_busy_storages.pop_back();
    pimpl->free_worker_ids.push(worker_id);
  }
}

BusyStorage::WorkerId BusyStorage::PopWorkerId() {
  WorkerId id{kInvalidWorkerId};
  if (!pimpl->free_worker_ids.pop(id)) {
    LOG_ERROR() << "Failed to obtain worker_id, load statistics is accounted "
                   "with some error";
  } else {
    this_thread_busy_storages.push_back(this);
  }
  return id;
}

Duration BusyStorage::GetNotCommittedLoad(WorkerId worker_id) const {
  const auto start_work = pimpl->start_work[worker_id];
  if (!start_work) {
    return Duration(0);
  } else {
    return utils::datetime::SteadyNow() - *start_work;
  }
}

}  // namespace utils::statistics

USERVER_NAMESPACE_END
