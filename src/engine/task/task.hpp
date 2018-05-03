#pragma once

#include <atomic>
#include <cassert>
#include <chrono>
#include <functional>
#include <memory>

#include <ev.h>
#include <boost/coroutine/symmetric_coroutine.hpp>

#include <engine/ev/thread_control.hpp>

namespace engine {

class TaskWorkerImpl;
class TaskProcessor;

class Task {
 public:
  explicit Task(TaskProcessor* task_processor);
  virtual ~Task();

  using CallType =
      typename boost::coroutines::symmetric_coroutine<Task*>::call_type;
  using YieldType =
      typename boost::coroutines::symmetric_coroutine<Task*>::yield_type;
  using WakeUpCb = std::function<void()>;

  enum class State { kQueued, kRunning, kWaiting, kComplete, kCanceled };

  enum class YieldState { kTaskWaiting, kTaskComplete };

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

  void SetYield(YieldType* yield) { yield_ = yield; }
  void SetYieldResult(YieldState state) { yield_state_ = state; }

  static void CoroFunc(YieldType& yield);

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

  CallType* coro_;
  YieldType* yield_;
  YieldState yield_state_;
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
