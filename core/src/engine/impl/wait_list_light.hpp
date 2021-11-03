#pragma once

#include <atomic>
#include <cstddef>

#include <boost/smart_ptr/intrusive_ptr.hpp>

USERVER_NAMESPACE_BEGIN

namespace engine::impl {

class TaskContext;

/// Wait list for a single entry. All functions are thread-safe.
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

  /// Create an empty `WaitListLight`
  WaitListLight() noexcept = default;

  WaitListLight(const WaitListLight&) = delete;
  WaitListLight(WaitListLight&&) = delete;
  WaitListLight& operator=(const WaitListLight&) = delete;
  WaitListLight& operator=(WaitListLight&&) = delete;
  ~WaitListLight();

  bool IsEmpty() const;

  /// @brief Append the task to the `WaitListLight`
  /// @note To account for `WakeupOne()` calls between condition check and
  /// `Sleep` + `Append`, you have to recheck the condition after `Append`
  /// returns in `SetupWakeups`.
  void Append(boost::intrusive_ptr<impl::TaskContext> context) noexcept;

  /// @brief Remove the task from the `WaitListLight` without wakeup
  void Remove(impl::TaskContext& context) noexcept;

  void WakeupOne();

 private:
  std::atomic<impl::TaskContext*> waiting_{nullptr};
#ifndef NDEBUG
  impl::TaskContext* owner_{nullptr};
#endif
  std::atomic<size_t> in_wakeup_{0};
};

}  // namespace engine::impl

USERVER_NAMESPACE_END
