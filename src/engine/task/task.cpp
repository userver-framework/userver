#include "task.hpp"

#include <cassert>

#include <engine/coro/pool.hpp>
#include <logging/log.hpp>

#include "task_processor.hpp"

namespace engine {

Task::Task(TaskProcessor* task_processor)
    : state_(State::kWaiting),
      task_processor_(task_processor),
      wake_up_cb_([this]() { WakeUp(); }),
      wait_state_(0),
      coro_(nullptr),
      yield_(nullptr),
      yield_state_(YieldState::kTaskWaiting) {}

Task::~Task() {
  if (coro_) {
    task_processor_->GetCoroPool().PutCoroutine(coro_);
    coro_ = nullptr;
  }
}

void Task::Fail() noexcept { LOG_ERROR() << "unexpected task Fail"; }

Task::State Task::RunTask() {
  if (!coro_) coro_ = task_processor_->GetCoroPool().GetCoroutine();

  wait_state_ = 0;
  (*coro_)(this);
  YieldState res = yield_state_;

  if (res == YieldState::kTaskComplete) {
    task_processor_->GetCoroPool().PutCoroutine(coro_);
    coro_ = nullptr;
    OnComplete();
    return State::kComplete;
  }
  return State::kWaiting;
}

void Task::Wait() {
  assert(yield_);
  Task* task = (*yield_)().get();
  static_cast<void>(task);
  assert(task == this);
}

void Task::Sleep() {
  if (wait_state_.fetch_or(1) == 2) task_processor_->AddTask(this);
}

ev::ThreadControl& Task::GetEventThread() {
  return GetTaskProcessor().EventThreadPool().NextThread();
}

void Task::CoroFunc(YieldType& yield) {
  Task* task = yield.get();
  for (;;) {
    assert(task != nullptr);
    task->SetYield(&yield);
    task->Run();
    task->SetYield(nullptr);
    task->SetYieldResult(YieldState::kTaskComplete);
    task = yield().get();
  }
}

void Task::WakeUp() {
  if (wait_state_.fetch_or(2) == 1) task_processor_->AddTask(this);
}

namespace current_task {
namespace {

thread_local Task* current_task_ptr = nullptr;

}  // namespace

Task* GetCurrentTask() {
  if (current_task_ptr == nullptr)
    throw std::logic_error(
        "current_task::GetCurrentTask() called outside coroutine");
  return current_task_ptr;
}

void SetCurrentTask(Task* task) { current_task_ptr = task; }

}  // namespace current_task
}  // namespace engine
