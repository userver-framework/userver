#pragma once

#include <chrono>
#include <condition_variable>
#include <list>
#include <memory>
#include <mutex>

#include <engine/ev/timer.hpp>

#include "notifier.hpp"

namespace engine {

template <typename CurrentTask>
class CondVar {
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
    UniqueNotifier(CondVar& cond_var, NotifierList::iterator it);
    ~UniqueNotifier();

    void Notify();

   private:
    CondVar& cond_var_;
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

template <typename CurrentTask>
void CondVar<CurrentTask>::Wait(std::unique_lock<std::mutex>& lock) {
  auto notifier = SaveNotifier();
  DoWait(lock);
}

template <typename CurrentTask>
template <typename Predicate>
void CondVar<CurrentTask>::Wait(std::unique_lock<std::mutex>& lock,
                                Predicate predicate) {
  if (predicate()) {
    return;
  }

  auto notifier = SaveNotifier();
  while (!predicate()) {
    DoWait(lock);
  }
}

template <typename CurrentTask>
template <typename Rep, typename Period>
std::cv_status CondVar<CurrentTask>::WaitFor(
    std::unique_lock<std::mutex>& lock,
    const std::chrono::duration<Rep, Period>& timeout) {
  if (timeout <= decltype(timeout)::zero()) {
    return std::cv_status::timeout;
  }

  return DoWaitFor(lock, timeout, [] { return true; });
}

template <typename CurrentTask>
template <typename Rep, typename Period, typename Predicate>
bool CondVar<CurrentTask>::WaitFor(
    std::unique_lock<std::mutex>& lock,
    const std::chrono::duration<Rep, Period>& timeout, Predicate predicate) {
  if (predicate()) {
    return true;
  }
  if (timeout <= decltype(timeout)::zero()) {
    return false;
  }
  return DoWaitFor(lock, timeout, std::move(predicate)) ==
         std::cv_status::no_timeout;
}

template <typename CurrentTask>
template <typename Clock, typename Duration>
std::cv_status CondVar<CurrentTask>::WaitUntil(
    std::unique_lock<std::mutex>& lock,
    const std::chrono::time_point<Clock, Duration>& until) {
  return WaitFor(lock, until - Clock::now());
}

template <typename CurrentTask>
template <typename Clock, typename Duration, typename Predicate>
bool CondVar<CurrentTask>::WaitUntil(
    std::unique_lock<std::mutex>& lock,
    const std::chrono::time_point<Clock, Duration>& until,
    Predicate predicate) {
  return WaitFor(lock, until - Clock::now(), std::move(predicate));
}

template <typename CurrentTask>
void CondVar<CurrentTask>::NotifyOne() {
  std::lock_guard<std::mutex> lock(notifiers_mutex_);
  if (!notifiers_.empty()) {
    notifiers_.front()->Notify();
  }
}

template <typename CurrentTask>
void CondVar<CurrentTask>::NotifyAll() {
  std::lock_guard<std::mutex> lock(notifiers_mutex_);
  for (auto* notifier : notifiers_) {
    notifier->Notify();
  }
}

template <typename CurrentTask>
CondVar<CurrentTask>::UniqueNotifier::UniqueNotifier(CondVar& cond_var,
                                                     NotifierList::iterator it)
    : cond_var_(cond_var), it_(it) {}

template <typename CurrentTask>
CondVar<CurrentTask>::UniqueNotifier::~UniqueNotifier() {
  std::lock_guard<std::mutex> lock(cond_var_.notifiers_mutex_);
  cond_var_.notifiers_.erase(it_);
}

template <typename CurrentTask>
void CondVar<CurrentTask>::UniqueNotifier::Notify() {
  (*it_)->Notify();
}

template <typename CurrentTask>
typename CondVar<CurrentTask>::UniqueNotifier
CondVar<CurrentTask>::SaveNotifier() {
  auto& notifier = CurrentTask::GetNotifier();
  NotifierList::iterator it;
  {
    std::lock_guard<std::mutex> lock(notifiers_mutex_);
    it = notifiers_.emplace_back(&notifier);
  }
  return UniqueNotifier(*this, it);
}

template <typename CurrentTask>
void CondVar<CurrentTask>::DoWait(std::unique_lock<std::mutex>& lock) {
  lock.unlock();
  CurrentTask::Wait();
  lock.lock();
}

template <typename CurrentTask>
template <typename Rep, typename Period, typename Predicate>
std::cv_status CondVar<CurrentTask>::DoWaitFor(
    std::unique_lock<std::mutex>& lock,
    const std::chrono::duration<Rep, Period>& timeout, Predicate predicate) {
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
