#include <userver/utils/statistics/busy.hpp>

#include <optional>
#include <thread>
#include <vector>

#include <userver/utils/datetime.hpp>

USERVER_NAMESPACE_BEGIN

namespace utils::statistics {

using Timer = utils::datetime::SteadyClock;
using Duration = BusyStorage::Duration;

struct BusyCounter {
  std::atomic<Duration> value{Duration{}};

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

struct BusyStorage::Impl {
  Impl(Duration epoch_duration, Duration history_period)
      : recent_period(epoch_duration, history_period) {}

  RecentPeriod<BusyCounter, BusyResult, Timer> recent_period;
  std::atomic<Timer::time_point> start_work{Timer::time_point{}};
  std::atomic<std::size_t> refcount{0};

#ifndef NDEBUG
  std::atomic<std::thread::id> thread_id{};
#endif
};

BusyStorage::BusyStorage(Duration epoch_duration, Duration history_period)
    : pimpl(std::make_unique<Impl>(epoch_duration, history_period)) {}

BusyStorage::~BusyStorage() {
  UASSERT_MSG(!IsAlreadyStarted(),
              "BusyStorage was not stopped before destruction");
}

double BusyStorage::GetCurrentLoad() const {
  pimpl->recent_period.UpdateEpochIfOld();

  const Duration current_load_duration = GetNotCommittedLoad();

  const auto result =
      pimpl->recent_period.GetStatsForPeriod(Timer::duration::min(), true);
  auto duration = result.Total();
  if (duration.count() == 0) duration = pimpl->recent_period.GetMaxDuration();

  auto load_duration = result.Get() + std::min(current_load_duration, duration);
  return static_cast<double>(load_duration.count()) / duration.count();
}

bool BusyStorage::IsAlreadyStarted() const noexcept {
  return pimpl->refcount.load() != 0;
}

void BusyStorage::StartWork() {
  if (++pimpl->refcount != 1) {
#ifndef NDEBUG
    UASSERT_MSG(pimpl->thread_id.load() == std::this_thread::get_id(),
                "BusyStorage should be started and stopped at the same thread");
#endif
    return;
  }

#ifndef NDEBUG
  pimpl->thread_id = std::this_thread::get_id();
#endif

  pimpl->start_work = utils::datetime::SteadyNow();
}

void BusyStorage::StopWork() noexcept {
  const auto refcount_snapshot = pimpl->refcount--;
  UASSERT_MSG(refcount_snapshot != 0, "Stop was called more times than start");
  if (refcount_snapshot != 1) {
#ifndef NDEBUG
    UASSERT_MSG(pimpl->thread_id.load() == std::this_thread::get_id(),
                "BusyStorage should be started and stopped at the same thread");
#endif
    return;
  }

  auto not_committed_load = GetNotCommittedLoad();
  auto& value = pimpl->recent_period.GetCurrentCounter().value;
  value = value.load() + not_committed_load;

  pimpl->start_work = Timer::time_point{};
}

Duration BusyStorage::GetNotCommittedLoad() const noexcept {
  const auto start_work = pimpl->start_work.load();
  if (start_work == Timer::time_point{}) {
    return Duration(0);
  } else {
    return utils::datetime::SteadyNow() - start_work;
  }
}

}  // namespace utils::statistics

USERVER_NAMESPACE_END
