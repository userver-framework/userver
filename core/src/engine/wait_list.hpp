#pragma once

#include <atomic>
#include <mutex>

#include <boost/smart_ptr/intrusive_ptr.hpp>
#include <utils/fast_pimpl.hpp>

#include <engine/wait_list_base.hpp>

namespace engine {
namespace impl {

class TaskContext;

class WaitList final : public WaitListBase {
 public:
  class Lock final : public WaitListBase::Lock {
   public:
    explicit Lock(WaitList& list) : impl_(list.mutex_) {}

    explicit operator bool() override { return !!impl_; }

    void Acquire() override { impl_.lock(); }
    void Release() override { impl_.unlock(); }

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

  ~WaitList() final;

  bool IsEmpty(WaitListBase::Lock&) const override;

  void Append(WaitListBase::Lock&,
              boost::intrusive_ptr<impl::TaskContext>) override;
  void WakeupOne(WaitListBase::Lock&) override;
  void WakeupAll(WaitListBase::Lock&) override;

  void Remove(const boost::intrusive_ptr<impl::TaskContext>&) override;

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

}  // namespace impl
}  // namespace engine
