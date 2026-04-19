#pragma once

#include <chrono>
#include <optional>
#include <string>
#include <string_view>

#include <cassandra.h>

#include <userver/tracing/span.hpp>

#include <storages/scylla/driver/cass_wrappers.hpp>
#include <storages/scylla/driver/prepared_cache.hpp>
#include <storages/scylla/driver/session_impl.hpp>
#include <storages/scylla/scylla_config.hpp>
#include <storages/scylla/stats.hpp>

USERVER_NAMESPACE_BEGIN

namespace storages::scylla::impl::driver {

struct RequestContext final {
    DriverSessionImpl& session;
    DriverSessionImpl::ConnPtr connection;
    CassSession* native_session;
    PreparedStatementCache& prepared_cache;
    stats::OperationStatisticsItem& stats;
    tracing::Span span;
    std::chrono::milliseconds request_timeout;
    std::optional<CommandControl> cc;
    std::string_view keyspace;
    std::string_view table;
};

RequestContext MakeTableRequestContext(
    DriverSessionImpl& session,
    std::string span_name,
    std::string_view keyspace,
    std::string_view table
);

RequestContext MakeSessionRequestContext(DriverSessionImpl& session, std::string span_name, std::string_view keyspace);

CassResultPtr ExecuteStatement(
    RequestContext& ctx,
    CassStatementPtr statement,
    bool idempotent,
    std::string_view action
);

void ExecuteBatch(RequestContext& ctx, CassBatchPtr batch, bool idempotent, std::string_view action);

}  // namespace storages::scylla::impl::driver

USERVER_NAMESPACE_END
