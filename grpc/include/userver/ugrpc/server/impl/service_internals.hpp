#pragma once

#include <userver/dynamic_config/source.hpp>
#include <userver/engine/task/task_processor_fwd.hpp>
#include <userver/logging/fwd.hpp>

#include <userver/ugrpc/server/middlewares/fwd.hpp>

USERVER_NAMESPACE_BEGIN

namespace ugrpc::impl {
class StatisticsStorage;
}  // namespace ugrpc::impl

namespace ugrpc::server::impl {

class CompletionQueuePool;

/// Config for a `ServiceWorker`, provided by `ugrpc::server::Server`
struct ServiceInternals final {
    CompletionQueuePool& completion_queues;
    engine::TaskProcessor& task_processor;
    ugrpc::impl::StatisticsStorage& statistics_storage;
    Middlewares middlewares;
    const dynamic_config::Source config_source;
};

}  // namespace ugrpc::server::impl

USERVER_NAMESPACE_END
