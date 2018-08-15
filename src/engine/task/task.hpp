#pragma once

#include <chrono>

#include <boost/smart_ptr/intrusive_ptr.hpp>

#include <engine/coro/pool.hpp>
#include <engine/ev/thread_control.hpp>
#include <engine/wait_helpers.hpp>

namespace engine {
namespace impl {
class TaskContext;
class TaskContextHolder;
}  // namespace impl

class TaskProcessor;

class Task {
 public:
  using CoroutinePtr = coro::Pool<Task>::CoroutinePtr;
  using TaskPipe = coro::Pool<Task>::TaskPipe;

  enum class Importance { kNormal, kCritical };

  enum class State {
    kInvalid,    // unusable
    kNew,        // just created, not registered with task processor
    kQueued,     // awaits dispatch
    kRunning,    // executing user code
    kWaiting,    // waiting for blocking call to complete
    kCancelled,  // exited user code because of external request
    kCompleted,  // exited user code with return or throw
  };

  enum class CancellationReason {
    kNone,
    kUserRequest,
    kOverload,
    kAbandoned,
    kShutdown,
  };

  Task();
  virtual ~Task();

  Task(Task&&) noexcept;
  Task& operator=(Task&&) noexcept;

  bool IsValid() const;
  explicit operator bool() const { return IsValid(); }

  State GetState() const;
  bool IsFinished() const;

  void Wait() const;

  template <typename Rep, typename Period>
  void WaitFor(const std::chrono::duration<Rep, Period>&) const;

  template <typename Clock, typename Duration>
  void WaitUntil(const std::chrono::time_point<Clock, Duration>&) const;

  void Detach() &&;
  void RequestCancel();
  CancellationReason GetCancellationReason() const;

 protected:
  Task(impl::TaskContextHolder&&);

 private:
  void DoWaitUntil(Deadline) const;

  boost::intrusive_ptr<impl::TaskContext> context_;
};

namespace current_task {

void CancellationPoint();

TaskProcessor& GetTaskProcessor();

ev::ThreadControl& GetEventThread();

}  // namespace current_task

template <typename Rep, typename Period>
void Task::WaitFor(const std::chrono::duration<Rep, Period>& duration) const {
  DoWaitUntil(MakeDeadline(duration));
}

template <typename Clock, typename Duration>
void Task::WaitUntil(
    const std::chrono::time_point<Clock, Duration>& until) const {
  DoWaitUntil(MakeDeadline(until));
}

}  // namespace engine
