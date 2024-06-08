#include <engine/task/task_counter.hpp>

#include <thread>

#include <userver/compiler/thread_local.hpp>
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

compiler::ThreadLocal local_task_counter_data = [] {
  return LocalTaskCounterData{};
};

}  // namespace

TaskCounter::Token::Token(TaskCounter& counter) noexcept
    : lock_(counter.tasks_alive_.Lock()) {
  concurrent::impl::AsymmetricThreadFenceLight();
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

void TaskCounter::WaitForExhaustionBlocking() const noexcept {
  while (MayHaveTasksAlive()) {
    std::this_thread::sleep_for(std::chrono::milliseconds{10});
  }
}

bool TaskCounter::MayHaveTasksAlive() const noexcept {
  concurrent::impl::AsymmetricThreadFenceHeavy();
  return !tasks_alive_.IsFree();
}

Rate TaskCounter::GetCreatedTasks() const noexcept {
  return Rate{tasks_alive_.GetAcquireCountApprox()};
}

Rate TaskCounter::GetDestroyedTasks() const noexcept {
  return Rate{tasks_alive_.GetReleaseCountApprox()};
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
  return GetApproximate(GlobalCounterId::kCancelOverload);
}

Rate TaskCounter::GetTasksOverload() const noexcept {
  return GetApproximate(GlobalCounterId::kOverload);
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
  Increment(GlobalCounterId::kCancelOverload);
}

void TaskCounter::AccountTaskOverload() noexcept {
  Increment(GlobalCounterId::kOverload);
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
  Rate total{global_counters_[static_cast<std::size_t>(id)].Read()};
  return total;
}

void TaskCounter::Increment(LocalCounterId id) noexcept {
  auto local_data = local_task_counter_data.Use();
  UASSERT(local_data->local_counter == this);
  auto& counter = (*local_counters_[local_data->task_processor_thread_index])
      [static_cast<std::size_t>(id)];
  counter.Store(counter.Load() + Rate{1});
}

void TaskCounter::Increment(GlobalCounterId id) noexcept {
  global_counters_[static_cast<std::size_t>(id)].Add(1);
}

void SetLocalTaskCounterData(TaskCounter& counter, std::size_t thread_id) {
  auto local_data = local_task_counter_data.Use();
  *local_data = {&counter, thread_id};
}

}  // namespace engine::impl

USERVER_NAMESPACE_END
