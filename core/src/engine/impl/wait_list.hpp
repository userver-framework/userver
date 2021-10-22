#pragma once

#include <atomic>
#include <mutex>

#include <boost/smart_ptr/intrusive_ptr.hpp>
#include <userver/utils/fast_pimpl.hpp>

USERVER_NAMESPACE_BEGIN

namespace engine::impl {

class TaskContext;

/// Wait list for multiple entries with explicit control over critical section.
class WaitList final {
 public:
  class Lock final {
   public:
    explicit Lock(WaitList& list) noexcept : impl_(list.mutex_) {}

    explicit operator bool() noexcept { return !!impl_; }

    void lock() { impl_.lock(); }
    void unlock() { impl_.unlock(); }

   private:
    std::unique_lock<std::mutex> impl_;
  };

  // This guard is used to optimize the hot path of unlocking:
  //
  // Use `WaitersScopeCounter` before acquiring the `WaitList::Lock` and do
  // not destroy it as long as the coroutine  may go to sleep.
  //
  // Now in the `unlock` part call `GetCountOfSleepies()` before
  // `WaitList::Lock + WakeupOne/WakeupAll`.
  class WaitersScopeCounter final {
   public:
    explicit WaitersScopeCounter(WaitList& list) noexcept : impl_(list) {
      ++impl_.sleepies_;
    }
    ~WaitersScopeCounter() { --impl_.sleepies_; }

   private:
    WaitList& impl_;
  };

  WaitList();
  WaitList(const WaitList&) = delete;
  WaitList(WaitList&&) = delete;
  WaitList& operator=(const WaitList&) = delete;
  WaitList& operator=(WaitList&&) = delete;

  ~WaitList();

  bool IsEmpty(Lock&) const;

  void Append(Lock&, boost::intrusive_ptr<impl::TaskContext>);
  void WakeupOne(Lock&);
  void WakeupAll(Lock&);

  void Remove(Lock&, boost::intrusive_ptr<impl::TaskContext>);

  // Returns the maximum amount of coroutines that may be sleeping.
  //
  // If the function returns 0 - there's no one asleep on this wait list.
  std::size_t GetCountOfSleepies() const noexcept { return sleepies_.load(); }

 private:
  std::atomic<std::size_t> sleepies_{0};
  std::mutex mutex_;

  struct List;
  static constexpr std::size_t kListSize = sizeof(void*) * 2;
  static constexpr std::size_t kListAlignment = alignof(void*);
  utils::FastPimpl<List, kListSize, kListAlignment> waiting_contexts_;
};

}  // namespace engine::impl

USERVER_NAMESPACE_END
