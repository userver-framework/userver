#pragma once

#include <userver/utils/fast_pimpl.hpp>

USERVER_NAMESPACE_BEGIN

namespace engine::impl {

class TaskContext;

/// Wait list for multiple waiters.
class WaitList final {
 public:
  /// Create an empty `WaitList`.
  WaitList();

  WaitList(const WaitList&) = delete;
  WaitList(WaitList&&) = delete;
  WaitList& operator=(const WaitList&) = delete;
  WaitList& operator=(WaitList&&) = delete;
  ~WaitList();

  void WakeupOne() noexcept;
  void WakeupAll() noexcept;

 private:
  friend class WaitScope;

  class Impl;
  utils::FastPimpl<Impl, 32, 8> impl_;
};

class WaitScope final {
 public:
  explicit WaitScope(WaitList& owner, TaskContext& context);

  WaitScope(WaitScope&&) = delete;
  WaitScope&& operator=(WaitScope&&) = delete;
  ~WaitScope();

  TaskContext& GetContext() const noexcept;

  void Append() noexcept;
  void Remove() noexcept;

 private:
  struct Impl;
  utils::FastPimpl<Impl, 48, 16> impl_;
};

}  // namespace engine::impl

USERVER_NAMESPACE_END
