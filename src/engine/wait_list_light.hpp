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

  ~WaitListLight();

  WaitListLight() = default;
  WaitListLight(const WaitListLight&) = delete;
  WaitListLight(WaitListLight&&) = delete;
  WaitListLight& operator=(const WaitListLight&) = delete;
  WaitListLight& operator=(WaitListLight&&) = delete;

  void PinToCurrentTask();
  void PinToTask(impl::TaskContext& ctx);

  void Append(WaitListBase::Lock&,
              boost::intrusive_ptr<impl::TaskContext>) override;
  void WakeupOne(WaitListBase::Lock&) override;
  void WakeupAll(WaitListBase::Lock&) override;

  void Remove(const boost::intrusive_ptr<impl::TaskContext>&) override;

 private:
  std::atomic<impl::TaskContext*> waiting_;
#ifndef NDEBUG
  impl::TaskContext* owner_;
#endif
};

}  // namespace impl
}  // namespace engine
