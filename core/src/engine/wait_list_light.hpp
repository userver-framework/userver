#pragma once

#include <atomic>

#include <boost/smart_ptr/intrusive_ptr.hpp>

namespace engine::impl {

class TaskContext;

/// Wait list for a single entry. All functions are thread safe
class WaitListLight final {
 public:
  class SingleUserGuard final {
   public:
#ifdef NDEBUG
    SingleUserGuard(WaitListLight&) {}
#else
    SingleUserGuard(WaitListLight&);
    ~SingleUserGuard();

   private:
    WaitListLight& wait_list_;
#endif
  };

  ~WaitListLight();

  WaitListLight() = default;
  WaitListLight(const WaitListLight&) = delete;
  WaitListLight(WaitListLight&&) = delete;
  WaitListLight& operator=(const WaitListLight&) = delete;
  WaitListLight& operator=(WaitListLight&&) = delete;

  bool IsEmpty() const;

  /* NOTE: there is a TOCTOU race between Wakeup*() and condition
   * check+Append(), you have to recheck whether the condition is true just
   * after Append() returns in exec_after_asleep.
   */
  void Append(boost::intrusive_ptr<impl::TaskContext>);
  void WakeupOne();

  void Remove(boost::intrusive_ptr<impl::TaskContext>);

 private:
  std::atomic<impl::TaskContext*> waiting_{nullptr};
#ifndef NDEBUG
  impl::TaskContext* owner_{nullptr};
#endif
  std::atomic<bool> in_wakeup_{false};
};

}  // namespace engine::impl
