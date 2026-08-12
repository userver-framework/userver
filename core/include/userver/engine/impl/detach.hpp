#pragma once

#include <userver/engine/task/task_base.hpp>
#include <userver/utils/impl/internal_tag_fwd.hpp>

USERVER_NAMESPACE_BEGIN

namespace engine::impl {

/// @brief Detaches `task`: drops the owning reference to its TaskContext without
/// cancelling or waiting for it. The task keeps running on its own, kept alive by
/// the engine reference baton until it finishes.
///
/// Unlike engine::DetachUnscopedUnsafe, the task is NOT adopted into the
/// TaskProcessor's detached-tasks pool, so there is no per-task bookkeeping. The
/// caller is responsible for any waiting and cancellation it needs.
///
/// The task will receive no cancellations on engine shutdown.
void DetachUnscopedUnsafeNoCancellationOnShutdown(utils::impl::InternalTag, TaskBase&& task) noexcept;

}  // namespace engine::impl

USERVER_NAMESPACE_END
