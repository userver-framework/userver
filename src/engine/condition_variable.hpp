#pragma once

#include <chrono>
#include <condition_variable>
#include <list>
#include <memory>
#include <mutex>

#include <engine/ev/timer.hpp>
#include <engine/task/task.hpp>

#include "notifier.hpp"

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
  using NotifierList = std::list<Notifier*>;

  class UniqueNotifier {
   public:
    UniqueNotifier(ConditionVariable& cond_var, NotifierList::iterator it);
    ~UniqueNotifier();

    void Notify();

   private:
    ConditionVariable& cond_var_;
    NotifierList::iterator it_;
  };

  UniqueNotifier SaveNotifier();

  void DoWait(std::unique_lock<std::mutex>& lock);

  template <typename Rep, typename Period, typename Predicate>
  std::cv_status DoWaitFor(std::unique_lock<std::mutex>& lock,
                           const std::chrono::duration<Rep, Period>& timeout,
                           Predicate predicate);

  std::mutex notifiers_mutex_;
  NotifierList notifiers_;
};

inline void ConditionVariable::Wait(std::unique_lock<std::mutex>& lock) {
  auto notifier = SaveNotifier();
  DoWait(lock);
}

template <typename Predicate>
void ConditionVariable::Wait(std::unique_lock<std::mutex>& lock,
                             Predicate predicate) {
  if (predicate()) {
    return;
  }

  auto notifier = SaveNotifier();
  while (!predicate()) {
    DoWait(lock);
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
  std::lock_guard<std::mutex> lock(notifiers_mutex_);
  if (!notifiers_.empty()) {
    notifiers_.front()->Notify();
  }
}

inline void ConditionVariable::NotifyAll() {
  std::lock_guard<std::mutex> lock(notifiers_mutex_);
  for (auto* notifier : notifiers_) {
    notifier->Notify();
  }
}

inline ConditionVariable::UniqueNotifier::UniqueNotifier(
    ConditionVariable& cond_var, NotifierList::iterator it)
    : cond_var_(cond_var), it_(it) {}

inline ConditionVariable::UniqueNotifier::~UniqueNotifier() {
  std::lock_guard<std::mutex> lock(cond_var_.notifiers_mutex_);
  cond_var_.notifiers_.erase(it_);
}

inline void ConditionVariable::UniqueNotifier::Notify() { (*it_)->Notify(); }

inline ConditionVariable::UniqueNotifier ConditionVariable::SaveNotifier() {
  auto& notifier = CurrentTask::GetNotifier();
  NotifierList::iterator it;
  {
    std::lock_guard<std::mutex> lock(notifiers_mutex_);
    it = notifiers_.emplace(notifiers_.end(), &notifier);
  }
  return UniqueNotifier(*this, it);
}

inline void ConditionVariable::DoWait(std::unique_lock<std::mutex>& lock) {
  lock.unlock();
  CurrentTask::Wait();
  lock.lock();
}

template <typename Rep, typename Period, typename Predicate>
std::cv_status ConditionVariable::DoWaitFor(
    std::unique_lock<std::mutex>& lock,
    const std::chrono::duration<Rep, Period>& timeout, Predicate predicate) {
  if (timeout <= std::chrono::duration<Rep, Period>::zero()) {
    return std::cv_status::timeout;
  }

  bool has_timed_out = false;

  auto notifier = SaveNotifier();
  ev::Timer<CurrentTask> timeout_timer(
      [&notifier, &has_timed_out] {
        has_timed_out = true;
        notifier.Notify();
      },
      timeout);

  while (!has_timed_out) {
    DoWait(lock);
    if (predicate()) {
      return std::cv_status::no_timeout;
    }
  }
  return std::cv_status::timeout;
}

}  // namespace engine
