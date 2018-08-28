#pragma once

#include <deque>
#include <mutex>

#include <boost/smart_ptr/intrusive_ptr.hpp>

namespace engine {
namespace impl {

class TaskContext;

class WaitList {
 public:
  class Lock {
   public:
    explicit Lock(WaitList& list) : impl_(list.mutex_) {}

    explicit operator bool() { return !!impl_; }

    void Acquire() { impl_.lock(); }
    void Release() { impl_.unlock(); }

   private:
    std::unique_lock<std::mutex> impl_;
  };

  WaitList() = default;
  WaitList(const WaitList&) = delete;
  WaitList(WaitList&&) = delete;
  WaitList& operator=(const WaitList&) = delete;
  WaitList& operator=(WaitList&&) = delete;

  void Append(Lock&, boost::intrusive_ptr<impl::TaskContext>);
  void WakeupOne(Lock&);
  void WakeupAll(Lock&);

  void Remove(Lock&, const boost::intrusive_ptr<impl::TaskContext>&);

 private:
  std::mutex mutex_;
  std::deque<boost::intrusive_ptr<impl::TaskContext>> waiting_contexts_;
};

}  // namespace impl
}  // namespace engine
