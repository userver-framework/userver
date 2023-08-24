#pragma once

#include <cstdint>
#include <utility>

#include <userver/engine/task/cancel.hpp>
#include <userver/engine/task/task.hpp>
#include <userver/utils/fast_pimpl.hpp>

USERVER_NAMESPACE_BEGIN

namespace engine::impl {

class TaskContext;

class DetachedTasksSyncBlock final {
 public:
  enum class StopMode { kCancelAndWait, kCancel };

  explicit DetachedTasksSyncBlock(StopMode stop_mode);

  DetachedTasksSyncBlock(const DetachedTasksSyncBlock&) = delete;
  DetachedTasksSyncBlock(DetachedTasksSyncBlock&&) = delete;
  ~DetachedTasksSyncBlock();

  void Add(TaskContext& context);
  void Add(Task&& task);

  void RequestCancellation(TaskCancellationReason reason) noexcept;

  std::int64_t ActiveTasksApprox() const noexcept;

  struct Token;

  static void Dispose(Token& token) noexcept;

 private:
  struct Impl;
  utils::FastPimpl<Impl, 48, 8> impl_;
};

}  // namespace engine::impl

USERVER_NAMESPACE_END
