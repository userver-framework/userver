#pragma once

#include <optional>
#include <string_view>

#include <grpcpp/completion_queue.h>
#include <grpcpp/server_context.h>

#include <userver/dynamic_config/fwd.hpp>
#include <userver/dynamic_config/snapshot.hpp>
#include <userver/logging/fwd.hpp>
#include <userver/tracing/in_place_span.hpp>
#include <userver/tracing/span.hpp>
#include <userver/utils/any_storage.hpp>

#include <userver/ugrpc/impl/statistics_scope.hpp>
#include <userver/ugrpc/server/impl/call_kind.hpp>
#include <userver/ugrpc/server/middlewares/fwd.hpp>
#include <userver/ugrpc/server/storage_context.hpp>

USERVER_NAMESPACE_BEGIN

namespace ugrpc::impl {
class StatisticsStorage;
}  // namespace ugrpc::impl

namespace ugrpc::server::impl {

// Non-templated dependencies of CallProcessor. Must be movable.
struct CallParams {
    grpc::ServerContext& server_context;
    const std::string_view call_name;
    const std::string_view service_name;
    const std::string_view method_name;
    ugrpc::impl::MethodStatistics& method_statistics;
    ugrpc::impl::StatisticsStorage& statistics_storage;
    std::optional<tracing::InPlaceSpan>& span_storage;
    const Middlewares& middlewares;
    const dynamic_config::Source& config_source;
};

// Non-templated state of CallProcessor. Can be non-movable.
struct CallState : CallParams {
    CallState(CallParams&& params, CallKind call_kind);

    tracing::Span& GetSpan();

    ugrpc::impl::RpcStatisticsScope statistics_scope;
    CallKind call_kind;
    std::optional<dynamic_config::Snapshot> config_snapshot;
    utils::AnyStorage<StorageContext> storage_context{};
};

}  // namespace ugrpc::server::impl

USERVER_NAMESPACE_END
