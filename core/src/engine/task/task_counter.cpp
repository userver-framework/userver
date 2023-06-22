#include <engine/task/task_counter.hpp>

#include <algorithm>
#include <thread>

#include <compiler/tls.hpp>
#include <userver/utils/assert.hpp>

USERVER_NAMESPACE_BEGIN

namespace engine::impl {

namespace {

static_assert(std::atomic<std::uint64_t>::is_always_lock_free);

using Rate = utils::statistics::Rate;

struct LocalTaskCounterData final {
  TaskCounter* local_counter{nullptr};
  std::size_t task_processor_thread_index{};
};

thread_local /*constinit*/ LocalTaskCounterData local_task_counter_data;

USERVER_PREVENT_TLS_CACHING LocalTaskCounterData
GetLocalTaskCounterData() noexcept {
  return local_task_counter_data;
}

}  // namespace

TaskCounter::Token::Token(TaskCounter& counter) noexcept : counter_(counter) {
  counter_.Increment(GlobalCounterId::kCreated);
}

TaskCounter::Token::~Token() {
  counter_.Increment(GlobalCounterId::kDestroyed);
}

TaskCounter::CoroToken::CoroToken(TaskCounter& counter) noexcept
    : counter_(&counter) {
  counter_->Increment(LocalCounterId::kStarted);
}

TaskCounter::CoroToken::~CoroToken() {
  if (counter_) counter_->Increment(LocalCounterId::kStopped);
}

TaskCounter::CoroToken::CoroToken(TaskCounter::CoroToken&& other) noexcept
    : counter_(std::exchange(other.counter_, nullptr)) {}

TaskCounter::CoroToken& TaskCounter::CoroToken::operator=(
    TaskCounter::CoroToken&& rhs) noexcept {
  counter_ = std::exchange(rhs.counter_, nullptr);
  return *this;
}

TaskCounter::TaskCounter(std::size_t thread_count)
    : local_counters_(thread_count) {}

TaskCounter::~TaskCounter() { UASSERT(!MayHaveTasksAlive()); }

void TaskCounter::WaitForExhaustion() const noexcept {
  while (MayHaveTasksAlive()) {
    std::this_thread::sleep_for(std::chrono::milliseconds{10});
  }
}

bool TaskCounter::MayHaveTasksAlive() const noexcept {
  const auto destroyed = GetApproximate(GlobalCounterId::kDestroyed);
  // Individual loads are relaxed, but we need to sequence 'created' loads after
  // 'destroyed' loads.
  std::atomic_thread_fence(std::memory_order_seq_cst);
  const auto created = GetApproximate(GlobalCounterId::kCreated);
  UASSERT(created >= destroyed);
  return created > destroyed;
}

Rate TaskCounter::GetCreatedTasks() const noexcept {
  return GetApproximate(GlobalCounterId::kCreated);
}

Rate TaskCounter::GetDestroyedTasks() const noexcept {
  return GetApproximate(GlobalCounterId::kDestroyed);
}

Rate TaskCounter::GetStartedTasks() const noexcept {
  return GetApproximate(LocalCounterId::kStarted);
}

Rate TaskCounter::GetStoppedTasks() const noexcept {
  return GetApproximate(LocalCounterId::kStopped);
}

Rate TaskCounter::GetCancelledTasks() const noexcept {
  return GetApproximate(LocalCounterId::kCancelled);
}

Rate TaskCounter::GetCancelledTasksOverload() const noexcept {
  return GetApproximate(LocalCounterId::kCancelOverload);
}

Rate TaskCounter::GetTasksOverload() const noexcept {
  return GetApproximate(LocalCounterId::kOverload);
}

Rate TaskCounter::GetTasksOverloadSensor() const noexcept {
  return GetApproximate(LocalCounterId::kOverloadSensor);
}

Rate TaskCounter::GetTasksNoOverloadSensor() const noexcept {
  return GetApproximate(LocalCounterId::kNoOverloadSensor);
}

Rate TaskCounter::GetTaskSwitchFast() const noexcept {
  return GetApproximate(LocalCounterId::kSwitchFast);
}

Rate TaskCounter::GetTaskSwitchSlow() const noexcept {
  return GetApproximate(LocalCounterId::kSwitchSlow);
}

Rate TaskCounter::GetSpuriousWakeups() const noexcept {
  return GetApproximate(LocalCounterId::kSpuriousWakeups);
}

void TaskCounter::AccountTaskCancel() noexcept {
  Increment(LocalCounterId::kCancelled);
}

void TaskCounter::AccountTaskCancelOverload() noexcept {
  Increment(LocalCounterId::kCancelOverload);
}

void TaskCounter::AccountTaskOverload() noexcept {
  Increment(LocalCounterId::kOverload);
}

void TaskCounter::AccountTaskOverloadSensor() noexcept {
  Increment(LocalCounterId::kOverloadSensor);
}

void TaskCounter::AccountTaskNoOverloadSensor() noexcept {
  Increment(LocalCounterId::kNoOverloadSensor);
}

void TaskCounter::AccountTaskSwitchFast() noexcept {
  Increment(LocalCounterId::kSwitchFast);
}

void TaskCounter::AccountTaskSwitchSlow() noexcept {
  Increment(LocalCounterId::kSwitchSlow);
}

void TaskCounter::AccountSpuriousWakeup() noexcept {
  Increment(LocalCounterId::kSpuriousWakeups);
}

Rate TaskCounter::GetApproximate(LocalCounterId id) const noexcept {
  Rate total;
  for (const auto& local_counters_block : local_counters_) {
    total += (*local_counters_block)[static_cast<std::size_t>(id)].Load();
  }
  return total;
}

Rate TaskCounter::GetApproximate(GlobalCounterId id) const noexcept {
  return Rate{global_counters_[static_cast<std::size_t>(id)]->Load()} +
         GetApproximate(static_cast<LocalCounterId>(id));
}

void TaskCounter::Increment(LocalCounterId id) noexcept {
  const auto local_data = GetLocalTaskCounterData();
  UASSERT(local_data.local_counter == this);
  auto& counter = (*local_counters_[local_data.task_processor_thread_index])
      [static_cast<std::size_t>(id)];
  counter.Store(counter.Load() + Rate{1});
}

void TaskCounter::Increment(GlobalCounterId id) noexcept {
  const auto local_data = GetLocalTaskCounterData();
  auto& counter =
      (local_data.local_counter == this)
          ? (*local_counters_[local_data.task_processor_thread_index])
                [static_cast<std::size_t>(id)]
          : *global_counters_[static_cast<std::size_t>(id)];
  // seq_cst synchronizes-with MayHaveTasksAlive.
  counter.Add(Rate{1}, std::memory_order_seq_cst);
}

void SetLocalTaskCounterData(TaskCounter& counter, std::size_t thread_id) {
  local_task_counter_data = {&counter, thread_id};
}

}  // namespace engine::impl

USERVER_NAMESPACE_END
