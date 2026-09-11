#include <ugrpc/impl/event_engine/executor.hpp>

#include <userver/engine/async.hpp>
#include <userver/logging/log.hpp>

USERVER_NAMESPACE_BEGIN

// clang-format off

namespace ugrpc::impl {

Executor::Executor(engine::TaskProcessor& task_processor, concurrent::BackgroundTaskStorageCore& background_task_storage)
    : task_processor_{task_processor}
    , background_task_storage_{background_task_storage}
{
    // LOG_DEBUG("Executor {} created", static_cast<void*>(this));
}

Executor::~Executor() {
    // LOG_DEBUG("Executor {} destroy", static_cast<void*>(this));
}

void Executor::Run(y_absl::AnyInvocable<void()> closure) {
    // LOG_DEBUG("Executor {} Run(closure)", static_cast<void*>(this));

    background_task_storage_
            .Detach(engine::CriticalAsyncNoTracing(task_processor_, [closure = std::move(closure)]() mutable {
                closure();
            }));
}

}  // namespace ugrpc::impl

// clang-format on

USERVER_NAMESPACE_END
