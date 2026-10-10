#pragma once

#include <chrono>
#include <memory>
#include <optional>

#include <storages/mongo/cdriver/pool_impl.hpp>
#include <storages/mongo/pool_impl.hpp>
#include <storages/mongo/stats.hpp>
#include <userver/dynamic_config/snapshot.hpp>
#include <userver/formats/bson/bson_builder.hpp>
#include <userver/tracing/span.hpp>
#include <userver/utils/function_ref.hpp>

USERVER_NAMESPACE_BEGIN

namespace storages::mongo::impl::cdriver {

struct RequestContextBase {
    std::shared_ptr<stats::OperationStatisticsItem> stats;
    dynamic_config::Snapshot dynamic_config;
    CDriverPoolImpl::BoundClientPtr client;
    tracing::Span span;
    std::optional<std::chrono::milliseconds> inherited_deadline;
};

CDriverPoolImpl& GetCDriverPool(const PoolImplPtr& pool_impl);

CDriverPoolImpl::BoundClientPtr AcquireClient(CDriverPoolImpl& pool, stats::OperationStatisticsItem& stats);

RequestContextBase MakeRequestContextBase(
    tracing::Span span,
    std::shared_ptr<stats::OperationStatisticsItem> stats,
    dynamic_config::Snapshot dynamic_config,
    utils::function_ref<CDriverPoolImpl::BoundClientPtr(stats::OperationStatisticsItem&)> get_client
);

std::chrono::milliseconds ComputeAdjustedMaxServerTime(
    std::chrono::milliseconds user_max_server_time,
    const RequestContextBase& context
);

void SetMaxServerTime(
    std::optional<formats::bson::impl::BsonBuilder>& builder,
    std::chrono::milliseconds max_server_time,
    const RequestContextBase& context
);

void PrepareCursorOptions(
    std::optional<formats::bson::impl::BsonBuilder>& options,
    std::chrono::milliseconds max_server_time,
    bool has_comment_option,
    const RequestContextBase& context
);

}  // namespace storages::mongo::impl::cdriver

USERVER_NAMESPACE_END
