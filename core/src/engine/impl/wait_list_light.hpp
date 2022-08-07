#pragma once

#include <boost/smart_ptr/intrusive_ptr.hpp>

#include <userver/utils/fast_pimpl.hpp>

USERVER_NAMESPACE_BEGIN

namespace engine::impl {

class TaskContext;

/// Wait list for a single entry. All functions are thread-safe.
class WaitListLight final {
 public:
  /// Create an empty `WaitListLight`
  WaitListLight() noexcept;

  WaitListLight(const WaitListLight&) = delete;
  WaitListLight(WaitListLight&&) = delete;
  WaitListLight& operator=(const WaitListLight&) = delete;
  WaitListLight& operator=(WaitListLight&&) = delete;
  ~WaitListLight();

  /// @brief Append the task to the `WaitListLight`
  /// @note To account for `WakeupOne()` calls between condition check and
  /// `Sleep` + `Append`, you have to recheck the condition after `Append`
  /// returns in `SetupWakeups`.
  void Append(boost::intrusive_ptr<impl::TaskContext> context) noexcept;

  /// @brief Remove the task from the `WaitListLight` without wakeup
  void Remove(impl::TaskContext& context) noexcept;

  /// @brief Removes the waiting task, if any, and wakes it up
  void WakeupOne();

 private:
  struct Impl;
  utils::FastPimpl<Impl, sizeof(void*) * 2, alignof(void*) * 2> impl_;
};

}  // namespace engine::impl

USERVER_NAMESPACE_END
