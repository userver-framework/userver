#pragma once

#include <atomic>
#include <cassert>
#include <chrono>
#include <functional>
#include <memory>

#include <ev.h>
#include <boost/coroutine/asymmetric_coroutine.hpp>

#include <engine/coro/pool.hpp>
#include <engine/ev/thread_control.hpp>

namespace engine {

class TaskProcessor;

class Task {
 public:
  explicit Task(TaskProcessor* task_processor);
  virtual ~Task();

  using CoroutinePtr = coro::Pool<Task>::CoroutinePtr;
  using TaskPipe = coro::Pool<Task>::TaskPipe;
  using WakeUpCb = std::function<void()>;

  enum class State { kQueued, kRunning, kWaiting, kComplete, kCanceled };

  enum class YieldReason { kTaskPending, kTaskWaiting, kTaskComplete };

  State GetState() const { return state_; }
  bool Finished() const {
    return state_ == State::kComplete || state_ == State::kCanceled;
  }

  virtual void Run() noexcept = 0;
  virtual void Fail() noexcept;
  virtual void OnComplete() noexcept = 0;

  State RunTask();
  void Wait();
  const WakeUpCb& GetWakeUpCb() const { return wake_up_cb_; }
  void Sleep();

  TaskProcessor& GetTaskProcessor() { return *task_processor_; }
  ev::ThreadControl& GetEventThread();

  void SetTaskPipe(TaskPipe* task_pipe) { task_pipe_ = task_pipe; }
  void SetYieldReason(YieldReason reason) { yield_reason_ = reason; }

  static void CoroFunc(TaskPipe& task_pipe);

 protected:
  void SetState(State state) { state_ = state; }

 private:
  friend class TaskProcessor;

  void WakeUp();

  State state_;

  TaskProcessor* task_processor_;
  const WakeUpCb wake_up_cb_;
  std::atomic<int> wait_state_;
  // (wait_state_ & 1): Sleep() called
  // (wait_state_ & 2): WakeUp() called

  CoroutinePtr coro_;
  TaskPipe* task_pipe_;
  YieldReason yield_reason_;
};

namespace current_task {

void SetCurrentTask(Task* task);
Task* GetCurrentTask();

inline ev::ThreadControl& GetEventThread() {
  return GetCurrentTask()->GetEventThread();
}

inline TaskProcessor& GetTaskProcessor() {
  return GetCurrentTask()->GetTaskProcessor();
}

inline void Wait() { return GetCurrentTask()->Wait(); }

inline const Task::WakeUpCb& GetWakeUpCb() {
  return GetCurrentTask()->GetWakeUpCb();
}

}  // namespace current_task
}  // namespace engine
