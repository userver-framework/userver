#pragma once

#include <atomic>
#include <deque>
#include <memory>
#include <mutex>

#include <boost/smart_ptr/intrusive_ptr.hpp>

#include <engine/wait_list_base.hpp>
#include <utils/assert.hpp>

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

  WaitList() = default;
  WaitList(const WaitList&) = delete;
  WaitList(WaitList&&) = delete;
  WaitList& operator=(const WaitList&) = delete;
  WaitList& operator=(WaitList&&) = delete;

#ifndef NDEBUG
  ~WaitList() {
    Lock lock{*this};
    SkipRemoved(lock);
    UASSERT_MSG(waiting_contexts_.empty(),
                "Someone is waiting on the WaitList");
  }
#else
  ~WaitList() = default;
#endif

  void Append(WaitListBase::Lock&,
              boost::intrusive_ptr<impl::TaskContext>) override;
  void WakeupOne(WaitListBase::Lock&) override;
  void WakeupAll(WaitListBase::Lock&) override;

  void Remove(const boost::intrusive_ptr<impl::TaskContext>&) override;

 private:
  void SkipRemoved(WaitListBase::Lock&);

  std::mutex mutex_;
  std::deque<boost::intrusive_ptr<impl::TaskContext>> waiting_contexts_;
};

}  // namespace impl
}  // namespace engine
