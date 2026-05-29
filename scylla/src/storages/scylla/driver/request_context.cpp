#include "request_context.hpp"

#include <chrono>
#include <exception>
#include <utility>

#include <userver/utils/assert.hpp>

#include <storages/scylla/driver/async_future.hpp>
#include <storages/scylla/driver/query_helpers.hpp>
#include <storages/scylla/driver/scylla_error.hpp>

USERVER_NAMESPACE_BEGIN

namespace storages::scylla::impl::driver {

namespace {

class QueryStopwatch final {
public:
    explicit QueryStopwatch(stats::OperationStatisticsItem& item) noexcept
        : item_(item), start_(std::chrono::steady_clock::now()) {}

    QueryStopwatch(const QueryStopwatch&) = delete;
    QueryStopwatch(QueryStopwatch&&) = delete;
    QueryStopwatch& operator=(const QueryStopwatch&) = delete;
    QueryStopwatch& operator=(QueryStopwatch&&) = delete;

    void AccountSuccess() noexcept {
        accounted_ = true;
        item_.Account(stats::ErrorType::kSuccess, Elapsed());
    }

    void AccountFailure(const std::exception& ex) noexcept {
        accounted_ = true;
        item_.Account(stats::ClassifyException(ex), Elapsed());
    }

    ~QueryStopwatch() {
        if (!accounted_) {
            item_.Account(stats::ErrorType::kOther, Elapsed());
        }
    }

private:
    std::chrono::milliseconds Elapsed() const noexcept {
        return std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::steady_clock::now() - start_);
    }

    stats::OperationStatisticsItem& item_;
    std::chrono::steady_clock::time_point start_;
    bool accounted_{false};
};

RequestContext MakeContextImpl(
    DriverSessionImpl& session,
    tracing::Span span,
    std::string_view keyspace,
    std::string_view table
) {
    const auto config_snapshot = session.GetConfig();
    auto cc = config_snapshot[kScyllaDefaultCommandControl].GetOptional(session.Id());
    auto timeout = session.GetSessionConfig().request_timeout;
    if (cc && cc->request_timeout) {
        timeout = *cc->request_timeout;
    }

    auto connection = session.GetActiveConnection();
    UASSERT(connection);
    auto* native_session = connection->GetSession();
    auto& prepared_cache = connection->GetPreparedCache();

    return RequestContext{
        session,
        std::move(connection),
        native_session,
        prepared_cache,
        *session.GetStatistics().session->queries,
        std::move(span),
        timeout,
        std::move(cc),
        keyspace,
        table,
    };
}

}  // namespace

RequestContext MakeTableRequestContext(
    DriverSessionImpl& session,
    std::string span_name,
    std::string_view keyspace,
    std::string_view table
) {
    return MakeContextImpl(session, MakeDbSpan(std::move(span_name), keyspace, table), keyspace, table);
}

RequestContext MakeSessionRequestContext(DriverSessionImpl& session, std::string span_name, std::string_view keyspace) {
    return MakeContextImpl(session, MakeDbSpan(std::move(span_name), keyspace), keyspace, {});
}

CassResultPtr ExecuteStatement(
    RequestContext& ctx,
    CassStatementPtr statement,
    bool idempotent,
    std::string_view action
) {
    ApplyConsistency(statement.get(), ctx.session.GetSessionConfig(), ctx.cc);
    ApplyDeadlineAndTimeout(statement.get(), ctx.request_timeout);
    if (idempotent) {
        MarkIdempotent(statement.get());
    }

    QueryStopwatch stopwatch(ctx.stats);
    CassFuturePtr future;
    try {
        future.reset(cass_session_execute(ctx.native_session, statement.get()));
        AsyncWaitFuture(future.get());
        CheckFuture(future.get(), action);
    } catch (const std::exception& ex) {
        stopwatch.AccountFailure(ex);
        throw;
    }
    stopwatch.AccountSuccess();

    return CassResultPtr{cass_future_get_result(future.get())};
}

void ExecuteBatch(RequestContext& ctx, CassBatchPtr batch, bool idempotent, std::string_view action) {
    const auto& session_config = ctx.session.GetSessionConfig();
    const auto consistency = ctx.cc && ctx.cc->consistency ? *ctx.cc->consistency : session_config.consistency;
    cass_batch_set_consistency(batch.get(), static_cast<CassConsistency>(consistency));

    const auto
        serial = ctx.cc && ctx.cc->serial_consistency ? *ctx.cc->serial_consistency : session_config.serial_consistency;
    cass_batch_set_serial_consistency(batch.get(), static_cast<CassConsistency>(serial));

    if (ctx.request_timeout.count() > 0) {
        cass_batch_set_request_timeout(batch.get(), static_cast<cass_uint64_t>(ctx.request_timeout.count()));
    }

    if (idempotent) {
        cass_batch_set_is_idempotent(batch.get(), cass_true);
    }

    QueryStopwatch stopwatch(ctx.stats);
    CassFuturePtr future;
    try {
        future.reset(cass_session_execute_batch(ctx.native_session, batch.get()));
        AsyncWaitFuture(future.get());
        CheckFuture(future.get(), action);
    } catch (const std::exception& ex) {
        stopwatch.AccountFailure(ex);
        throw;
    }
    stopwatch.AccountSuccess();
}

}  // namespace storages::scylla::impl::driver

USERVER_NAMESPACE_END
