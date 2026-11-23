#pragma once

#include <cstddef>
#include <optional>
#include <string_view>

#include <grpcpp/completion_queue.h>
#include <grpcpp/server_context.h>

#include <userver/dynamic_config/fwd.hpp>
#include <userver/dynamic_config/snapshot.hpp>
#include <userver/tracing/in_place_span.hpp>
#include <userver/utils/any_storage.hpp>

#include <userver/ugrpc/impl/statistics_scope.hpp>
#include <userver/ugrpc/rpc_type.hpp>
#include <userver/ugrpc/server/middlewares/fwd.hpp>
#include <userver/ugrpc/server/storage_context.hpp>

USERVER_NAMESPACE_BEGIN

namespace ugrpc::impl {
class StatisticsStorage;
}  // namespace ugrpc::impl

namespace ugrpc::server::impl {

struct ServiceTracingConfig;

// Non-templated dependencies of CallProcessor. Must be movable.
struct CallParams {
    grpc::ServerContextBase& server_context;
    RpcType rpc_type{};
    const std::string_view call_name;
    const std::string_view service_name;
    const std::string_view method_name;
    ugrpc::impl::MethodStatistics& method_statistics;
    ugrpc::impl::StatisticsStorage& statistics_storage;
    const Middlewares& middlewares;
    const dynamic_config::Source& config_source;
    const ServiceTracingConfig& tracing_config;
};

// Non-templated state of CallProcessor. Can be non-movable.
struct CallState : CallParams {
    explicit CallState(CallParams&& params);

    tracing::Span& GetSpan() noexcept {
        UASSERT(span);
        return span->Get();
    }

    std::optional<tracing::InPlaceSpan> span;
    ugrpc::impl::RpcStatisticsScope statistics_scope;
    // Wire size of the request message most recently received. Written only by the read side.
    std::optional<std::size_t> latest_request_size_bytes;
    // Wire size of the response message most recently sent. Written only by the write side.
    std::optional<std::size_t> latest_response_size_bytes;
    std::optional<dynamic_config::Snapshot> config_snapshot;
    utils::AnyStorage<StorageContext> storage_context{};
};

}  // namespace ugrpc::server::impl

USERVER_NAMESPACE_END
