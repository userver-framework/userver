#pragma once

#include <ev.h>
#include <atomic>
#include <cassert>
#include <chrono>
#include <functional>
#include <memory>

#include <boost/coroutine/symmetric_coroutine.hpp>

#include <engine/ev/thread_control.hpp>
#include <engine/notifier.hpp>

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
  Notifier& GetNotifier() { return wait_notifier_; }
  void Sleep();

  TaskProcessor& GetTaskProcessor() { return *task_processor_; }
  ev::ThreadControl& GetSchedulerThread();

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
  Notifier wait_notifier_;
  std::atomic<int> wait_state_;
  // (wait_state_ & 1): Sleep() called
  // (wait_state_ & 2): WakeUp() called

  CallType* coro_;
  YieldType* yield_;
  YieldState yield_state_;
};

class CurrentTask {
 public:
  static ev::ThreadControl& GetSchedulerThread() {
    return GetCurrentTask()->GetSchedulerThread();
  }

  static TaskProcessor& GetTaskProcessor() {
    return GetCurrentTask()->GetTaskProcessor();
  }

  static void Wait() { return GetCurrentTask()->Wait(); }

  static Notifier& GetNotifier() { return GetCurrentTask()->GetNotifier(); }

  static Task* GetCurrentTask();
  static void SetCurrentTask(Task* task);
};

}  // namespace engine
