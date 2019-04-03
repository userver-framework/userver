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

  WaitList();
  WaitList(const WaitList&) = delete;
  WaitList(WaitList&&) = delete;
  WaitList& operator=(const WaitList&) = delete;
  WaitList& operator=(WaitList&&) = delete;

  ~WaitList() final;

  void Append(WaitListBase::Lock&,
              boost::intrusive_ptr<impl::TaskContext>) override;
  void WakeupOne(WaitListBase::Lock&) override;
  void WakeupAll(WaitListBase::Lock&) override;

  void Remove(const boost::intrusive_ptr<impl::TaskContext>&) override;

 private:
  std::mutex mutex_;

  struct List;
  static constexpr std::size_t kListSize = sizeof(void*) * 2;
  static constexpr std::size_t kListAlignment = alignof(void*);
  utils::FastPimpl<List, kListSize, kListAlignment> waiting_contexts_;
};

}  // namespace impl
}  // namespace engine
