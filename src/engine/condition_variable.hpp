#pragma once

#include <atomic>
#include <chrono>
#include <condition_variable>
#include <list>
#include <mutex>

#include <engine/task/task.hpp>

#include "timer_event.hpp"

namespace engine {

class ConditionVariable {
 public:
  void Wait(std::unique_lock<std::mutex>& lock);

  template <typename Predicate>
  void Wait(std::unique_lock<std::mutex>& lock, Predicate predicate);

  template <typename Rep, typename Period>
  std::cv_status WaitFor(std::unique_lock<std::mutex>& lock,
                         const std::chrono::duration<Rep, Period>& timeout);

  template <typename Rep, typename Period, typename Predicate>
  bool WaitFor(std::unique_lock<std::mutex>& lock,
               const std::chrono::duration<Rep, Period>& timeout,
               Predicate predicate);

  template <typename Clock, typename Duration>
  std::cv_status WaitUntil(
      std::unique_lock<std::mutex>& lock,
      const std::chrono::time_point<Clock, Duration>& until);

  template <typename Clock, typename Duration, typename Predicate>
  bool WaitUntil(std::unique_lock<std::mutex>& lock,
                 const std::chrono::time_point<Clock, Duration>& until,
                 Predicate predicate);

  void NotifyOne();
  void NotifyAll();

 private:
  using WakeUpCbList = std::list<const Task::WakeUpCb*>;

  class WaitListItemDescriptor {
   public:
    WaitListItemDescriptor(ConditionVariable& cond_var,
                           WakeUpCbList::iterator it);
    ~WaitListItemDescriptor();

    void Notify() const;

   private:
    ConditionVariable& cond_var_;
    WakeUpCbList::iterator it_;
  };

  class CvTimer;

  WaitListItemDescriptor AddCurrentTaskToWaitList();

  void DoWait(std::unique_lock<std::mutex>& lock);

  template <typename Rep, typename Period, typename Predicate>
  std::cv_status DoWaitFor(std::unique_lock<std::mutex>& lock,
                           const std::chrono::duration<Rep, Period>& timeout,
                           Predicate predicate);

  std::mutex wait_list_mutex_;
  WakeUpCbList wait_list_;
};

namespace impl {

class UnlockScope {
 public:
  UnlockScope(std::unique_lock<std::mutex>& lock) : lock_(lock) {
    lock_.unlock();
  }

  ~UnlockScope() { lock_.lock(); }

 private:
  std::unique_lock<std::mutex>& lock_;
};

class LockScope {
 public:
  LockScope(std::unique_lock<std::mutex>& lock) : lock_(lock) { lock_.lock(); }

  ~LockScope() { lock_.unlock(); }

 private:
  std::unique_lock<std::mutex>& lock_;
};

}  // namespace impl

inline void ConditionVariable::Wait(std::unique_lock<std::mutex>& lock) {
  impl::UnlockScope unlock_scope(lock);
  auto wake_up_cb = AddCurrentTaskToWaitList();
  {
    impl::LockScope lock_scope(lock);
    DoWait(lock);
  }
}

template <typename Predicate>
void ConditionVariable::Wait(std::unique_lock<std::mutex>& lock,
                             Predicate predicate) {
  if (predicate()) {
    return;
  }

  impl::UnlockScope unlock_scope(lock);
  auto wake_up_cb = AddCurrentTaskToWaitList();
  {
    impl::LockScope lock_scope(lock);
    while (!predicate()) {
      DoWait(lock);
    }
  }
}

template <typename Rep, typename Period>
std::cv_status ConditionVariable::WaitFor(
    std::unique_lock<std::mutex>& lock,
    const std::chrono::duration<Rep, Period>& timeout) {
  return DoWaitFor(lock, timeout, [] { return true; });
}

template <typename Rep, typename Period, typename Predicate>
bool ConditionVariable::WaitFor(
    std::unique_lock<std::mutex>& lock,
    const std::chrono::duration<Rep, Period>& timeout, Predicate predicate) {
  if (predicate()) {
    return true;
  }
  return DoWaitFor(lock, timeout, std::move(predicate)) ==
         std::cv_status::no_timeout;
}

template <typename Clock, typename Duration>
std::cv_status ConditionVariable::WaitUntil(
    std::unique_lock<std::mutex>& lock,
    const std::chrono::time_point<Clock, Duration>& until) {
  return WaitFor(lock, until - Clock::now());
}

template <typename Clock, typename Duration, typename Predicate>
bool ConditionVariable::WaitUntil(
    std::unique_lock<std::mutex>& lock,
    const std::chrono::time_point<Clock, Duration>& until,
    Predicate predicate) {
  return WaitFor(lock, until - Clock::now(), std::move(predicate));
}

inline void ConditionVariable::NotifyOne() {
  std::lock_guard<std::mutex> lock(wait_list_mutex_);
  if (!wait_list_.empty()) {
    (*wait_list_.front())();
  }
}

inline void ConditionVariable::NotifyAll() {
  std::lock_guard<std::mutex> lock(wait_list_mutex_);
  for (auto* wake_up_cb : wait_list_) {
    (*wake_up_cb)();
  }
}

inline ConditionVariable::WaitListItemDescriptor::WaitListItemDescriptor(
    ConditionVariable& cond_var, WakeUpCbList::iterator it)
    : cond_var_(cond_var), it_(it) {}

inline ConditionVariable::WaitListItemDescriptor::~WaitListItemDescriptor() {
  std::lock_guard<std::mutex> lock(cond_var_.wait_list_mutex_);
  cond_var_.wait_list_.erase(it_);
}

inline void ConditionVariable::WaitListItemDescriptor::Notify() const {
  (**it_)();
}

inline ConditionVariable::WaitListItemDescriptor
ConditionVariable::AddCurrentTaskToWaitList() {
  const auto& wake_up_cb = current_task::GetWakeUpCb();
  WakeUpCbList::iterator it;
  {
    std::lock_guard<std::mutex> lock(wait_list_mutex_);
    it = wait_list_.emplace(wait_list_.end(), &wake_up_cb);
  }
  return WaitListItemDescriptor(*this, it);
}

class ConditionVariable::CvTimer {
 public:
  struct TimeoutState {
    std::mutex mutex;
    bool has_timed_out{false};
  };

  template <typename Rep, typename Period>
  CvTimer(ConditionVariable& cv,
          const std::chrono::duration<Rep, Period>& timeout)
      : waiting_item_(cv.AddCurrentTaskToWaitList()),
        state_(std::make_shared<TimeoutState>()),
        timer_([ state = state_, this ] { OnTimerCb(*state); }, timeout) {}

  ~CvTimer() {
    std::lock_guard<std::mutex> lock(state_->mutex);
    // prevents access to destroyed CvTimer
    state_->has_timed_out = true;
  }

  bool HasTimedOut() const { return state_->has_timed_out; }

 private:
  void OnTimerCb(TimeoutState& state) const;

  WaitListItemDescriptor waiting_item_;
  std::shared_ptr<TimeoutState> state_;
  impl::TimerEvent timer_;
};

inline void ConditionVariable::CvTimer::OnTimerCb(TimeoutState& state) const {
  std::lock_guard<std::mutex> lock(state.mutex);
  // prevents access to destroyed CvTimer's this
  if (state.has_timed_out) return;
  state.has_timed_out = true;
  waiting_item_.Notify();
}

inline void ConditionVariable::DoWait(std::unique_lock<std::mutex>& lock) {
  impl::UnlockScope unlock_scope(lock);
  current_task::Wait();
}

template <typename Rep, typename Period, typename Predicate>
std::cv_status ConditionVariable::DoWaitFor(
    std::unique_lock<std::mutex>& lock,
    const std::chrono::duration<Rep, Period>& timeout, Predicate predicate) {
  impl::UnlockScope unlock_scope(lock);
  if (timeout <= std::chrono::duration<Rep, Period>::zero()) {
    return std::cv_status::timeout;
  }

  CvTimer timer(*this, timeout);

  {
    impl::LockScope lock_scope(lock);
    while (!timer.HasTimedOut()) {
      DoWait(lock);
      if (predicate()) {
        return std::cv_status::no_timeout;
      }
    }
  }
  return std::cv_status::timeout;
}

}  // namespace engine
