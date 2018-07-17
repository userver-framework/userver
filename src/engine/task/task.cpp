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
      task_pipe_(nullptr),
      yield_reason_(YieldReason::kTaskPending) {}

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

  if (yield_reason_ == YieldReason::kTaskComplete) {
    task_processor_->GetCoroPool().PutCoroutine(coro_);
    coro_ = nullptr;
    OnComplete();
    return State::kComplete;
  }
  return State::kWaiting;
}

void Task::Wait() {
  assert(task_pipe_);
  yield_reason_ = YieldReason::kTaskWaiting;
  Task* task = (*task_pipe_)().get();
  static_cast<void>(task);
  assert(task == this);
}

void Task::Sleep() {
  if (wait_state_.fetch_or(1) == 2) task_processor_->AddTask(this);
}

ev::ThreadControl& Task::GetEventThread() {
  return GetTaskProcessor().EventThreadPool().NextThread();
}

void Task::CoroFunc(TaskPipe& task_pipe) {
  for (Task* task : task_pipe) {
    assert(task != nullptr);
    task->SetTaskPipe(&task_pipe);
    task->Run();
    task->SetTaskPipe(nullptr);
    task->SetYieldReason(YieldReason::kTaskComplete);
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
