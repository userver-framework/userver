#pragma once

#include "wait_list.hpp"

namespace engine {
namespace impl {

class WaitListLight final : public WaitListBase {
 public:
  class Lock final : public WaitListBase::Lock {
   public:
    Lock() = default;

    explicit operator bool() override;

    void Acquire() override {}
    void Release() override {}
  };

  ~WaitListLight() final;

  WaitListLight() = default;
  WaitListLight(const WaitListLight&) = delete;
  WaitListLight(WaitListLight&&) = delete;
  WaitListLight& operator=(const WaitListLight&) = delete;
  WaitListLight& operator=(WaitListLight&&) = delete;

  void PinToCurrentTask();
  void PinToTask(impl::TaskContext& ctx);

  /* NOTE: there is a TOCTOU race between Wakeup*() and condition
   * check+Append(), you have to recheck whether the condition is true just
   * after Append() returns in exec_after_asleep.
   */
  void Append(WaitListBase::Lock&,
              boost::intrusive_ptr<impl::TaskContext>) override;
  void WakeupOne(WaitListBase::Lock&) override;
  void WakeupAll(WaitListBase::Lock&) override;

  void Remove(const boost::intrusive_ptr<impl::TaskContext>&) override;

 private:
  std::atomic<impl::TaskContext*> waiting_{nullptr};
#ifndef NDEBUG
  impl::TaskContext* owner_{nullptr};
#endif
  std::atomic<bool> in_wakeup_{false};
};

}  // namespace impl
}  // namespace engine
