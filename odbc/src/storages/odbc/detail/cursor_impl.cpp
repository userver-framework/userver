#include <storages/odbc/detail/cursor_impl.hpp>

#include <algorithm>
#include <atomic>
#include <optional>
#include <utility>

#include <storages/odbc/detail/conn_ptr.hpp>
#include <storages/odbc/detail/connection.hpp>
#include <storages/odbc/detail/deadline.hpp>
#include <storages/odbc/detail/pool.hpp>
#include <storages/odbc/detail/statement_stats_storage.hpp>
#include <storages/odbc/detail/tracing.hpp>
#include <userver/storages/odbc/exception.hpp>
#include <userver/storages/odbc/impl/tracing_tags.hpp>
#include <userver/tracing/span.hpp>
#include <userver/tracing/tags.hpp>

USERVER_NAMESPACE_BEGIN

namespace storages::odbc::detail {

namespace {

engine::Deadline MakeCursorDeadline(
    std::chrono::milliseconds network_timeout,
    std::chrono::milliseconds statement_timeout
) {
    return std::min(GetExecuteDeadline(network_timeout), GetExecuteDeadline(statement_timeout));
}

class CursorMetrics final {
public:
    CursorMetrics(Query query, std::shared_ptr<Pool> pool)
        : query_{std::move(query)},
          pool_{std::move(pool)},
          statement_stats_storage_{&pool_->GetStatementStatsStorage()},
          statement_name_{
              query_.GetOptionalNameView()
                  ? std::optional<std::string>{std::string{*query_.GetOptionalNameView()}}
                  : std::nullopt
          },
          statement_generation_{statement_stats_storage_->GetGenerationToken()}
    {
        if (!statement_name_ || !statement_generation_) {
            statement_stats_storage_ = nullptr;
        }
    }

    void Finish(CursorTerminalResult result, std::chrono::microseconds busy_time) noexcept {
        if (accounted_.exchange(true)) {
            return;
        }
        if (result == CursorTerminalResult::kSuccess) {
            pool_->AccountQueryExecuted(busy_time);
        } else {
            if (result == CursorTerminalResult::kTimeout) {
                pool_->AccountQueryTimeout();
            } else {
                pool_->AccountQueryError();
            }
        }
        if (statement_stats_storage_) {
            const auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(busy_time);
            statement_stats_storage_->Account(
                *statement_name_,
                static_cast<std::size_t>(std::max<std::int64_t>(0, duration.count())),
                result == CursorTerminalResult::kSuccess
                    ? StatementStatsStorage::ExecutionResult::kSuccess
                    : StatementStatsStorage::ExecutionResult::kError,
                *statement_generation_
            );
        }
    }

private:
    Query query_;
    std::shared_ptr<Pool> pool_;
    StatementStatsStorage* statement_stats_storage_;
    std::optional<std::string> statement_name_;
    std::optional<StatementStatsStorage::GenerationToken> statement_generation_;
    std::atomic<bool> accounted_{false};
};

void AddQueryTags(USERVER_NAMESPACE::tracing::Span& span, const Query& query) {
    span.AddTag(USERVER_NAMESPACE::tracing::kDatabaseType, "odbc");
    const auto tags = tracing::MakeQuerySpanTags(query);
    if (tags.statement_name) {
        span.AddTag(USERVER_NAMESPACE::tracing::kDatabaseStatementName, std::string{*tags.statement_name});
    }
    if (tags.statement) {
        span.AddTag(USERVER_NAMESPACE::tracing::kDatabaseStatement, std::string{*tags.statement});
    }
}

}  // namespace

struct CursorImpl::Impl final {
    Impl(
        std::optional<ConnectionPtr> owned_connection,
        Query query,
        std::chrono::milliseconds network_timeout,
        std::chrono::milliseconds statement_timeout
    )
        : owned_connection{std::move(owned_connection)},
          query{std::move(query)},
          network_timeout{network_timeout},
          statement_timeout{statement_timeout}
    {}

    ~Impl() noexcept {
        Connection::CloseCursor(lease);
        owned_connection.reset();
    }

    void ReleaseOwnedConnectionIfTerminal() {
        if (lease.statement->terminal.load()) {
            owned_connection.reset();
        }
    }

    std::optional<ConnectionPtr> owned_connection;
    CursorLease lease;
    Query query;
    std::chrono::milliseconds network_timeout;
    std::chrono::milliseconds statement_timeout;
};

std::shared_ptr<CursorImpl> CursorImpl::Create(
    ConnectionPtr&& connection,
    std::shared_ptr<Pool> pool,
    Query query,
    const impl::ParameterList& parameters,
    std::chrono::milliseconds network_timeout,
    std::chrono::milliseconds statement_timeout,
    bool in_transaction
) {
    auto metrics = std::make_shared<CursorMetrics>(query, std::move(pool));
    auto impl = std::make_unique<
        Impl>(std::optional<ConnectionPtr>{std::move(connection)}, query, network_timeout, statement_timeout);
    impl->lease =
        (*impl->owned_connection)
            ->OpenCursor(
                query,
                parameters,
                MakeCursorDeadline(network_timeout, statement_timeout),
                in_transaction,
                [metrics = std::move(metrics)](CursorTerminalResult result, std::chrono::microseconds busy_time)
                    noexcept { metrics->Finish(result, busy_time); }
            );
    return std::shared_ptr<CursorImpl>{new CursorImpl{std::move(impl)}};
}

std::shared_ptr<CursorImpl> CursorImpl::Create(
    Connection& connection,
    std::shared_ptr<Pool> pool,
    Query query,
    const impl::ParameterList& parameters,
    std::chrono::milliseconds network_timeout,
    std::chrono::milliseconds statement_timeout,
    bool in_transaction
) {
    auto metrics = std::make_shared<CursorMetrics>(query, std::move(pool));
    auto impl = std::make_unique<Impl>(std::nullopt, query, network_timeout, statement_timeout);
    impl->lease = connection.OpenCursor(
        query,
        parameters,
        MakeCursorDeadline(network_timeout, statement_timeout),
        in_transaction,
        [metrics = std::move(metrics)](CursorTerminalResult result, std::chrono::microseconds busy_time) noexcept {
            metrics->Finish(result, busy_time);
        }
    );
    return std::shared_ptr<CursorImpl>{new CursorImpl{std::move(impl)}};
}

CursorImpl::CursorImpl(std::unique_ptr<Impl> impl) noexcept : impl_{std::move(impl)} {}

CursorImpl::~CursorImpl() noexcept = default;

ResultSet CursorImpl::Fetch(std::size_t rows) {
    if (rows == 0) {
        throw LogicError("ODBC cursor Fetch row count must be greater than zero");
    }
    if (Done()) {
        throw LogicError("ODBC cursor is terminal");
    }

    USERVER_NAMESPACE::tracing::Span span{"odbc_cursor_fetch"};
    AddQueryTags(span, impl_->query);
    try {
        auto result = Connection::FetchCursor(
            impl_->lease,
            rows,
            MakeCursorDeadline(impl_->network_timeout, impl_->statement_timeout)
        );
        span.AddTag("odbc_cursor_rows", result.Size());
        span.AddTag("odbc_cursor_total_rows", FetchedSoFar());
        span.AddTag("odbc_cursor_done", Done());
        impl_->ReleaseOwnedConnectionIfTerminal();
        return result;
    } catch (const std::exception& ex) {
        span.AddTag(USERVER_NAMESPACE::tracing::kErrorFlag, true);
        span.AddTag(USERVER_NAMESPACE::tracing::kErrorMessage, ex.what());
        impl_->ReleaseOwnedConnectionIfTerminal();
        throw;
    }
}

bool CursorImpl::Done() const noexcept { return impl_->lease.statement->terminal.load(); }

std::size_t CursorImpl::FetchedSoFar() const noexcept { return impl_->lease.statement->fetched_so_far.load(); }

}  // namespace storages::odbc::detail

USERVER_NAMESPACE_END
