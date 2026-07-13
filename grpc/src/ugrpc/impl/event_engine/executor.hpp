#pragma once

#include <grpc/event_engine/event_engine.h>

#include <userver/concurrent/background_task_storage.hpp>
#include <userver/engine/task/task_processor_fwd.hpp>

USERVER_NAMESPACE_BEGIN

namespace ugrpc::impl {

class Executor final {
public:
    Executor(engine::TaskProcessor& task_processor, concurrent::BackgroundTaskStorageCore& background_task_storage);

    ~Executor();

    void Run(y_absl::AnyInvocable<void()> closure);

private:
    engine::TaskProcessor& task_processor_;

    concurrent::BackgroundTaskStorageCore& background_task_storage_;
};

}  // namespace ugrpc::impl

USERVER_NAMESPACE_END
