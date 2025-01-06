#include <cctype>
#include <storages/postgres/detail/connection_impl.hpp>

#include <boost/functional/hash.hpp>

#include <string>
#include <string_view>
#include <userver/error_injection/hook.hpp>
#include <userver/logging/log.hpp>
#include <userver/testsuite/testpoint.hpp>
#include <userver/tracing/span.hpp>
#include <userver/tracing/tags.hpp>
#include <userver/utils/assert.hpp>
#include <userver/utils/datetime.hpp>
#include <userver/utils/rand.hpp>
#include <userver/utils/scope_guard.hpp>
#include <userver/utils/text_light.hpp>
#include <userver/utils/trivial_map.hpp>
#include <userver/utils/uuid4.hpp>

#include <storages/postgres/detail/tracing_tags.hpp>
#include <storages/postgres/experiments.hpp>
#include <storages/postgres/io/pg_type_parsers.hpp>
#include <userver/storages/postgres/exceptions.hpp>

USERVER_NAMESPACE_BEGIN

using USERVER_NAMESPACE::utils::RandRange;
using USERVER_NAMESPACE::utils::ScopeGuard;
using USERVER_NAMESPACE::utils::datetime::SteadyNow;
using USERVER_NAMESPACE::utils::text::ICaseStartsWith;

namespace storages::postgres::detail {

namespace {

constexpr std::string_view kStatementTimeoutParameter = "statement_timeout";
constexpr std::string_view kStatementVacuum = "vacuum";
constexpr std::string_view kStatementListen = "listen {}";
constexpr std::string_view kStatementUnlisten = "unlisten {}";

const Query kSetConfigQuery{fmt::format("SELECT set_config($1, $2, $3) as {}", kSetConfigQueryResultName)};

// we hope lc_messages is en_US, we don't control it anyway
const std::string kBadCachedPlanErrorMessage = "cached plan must not change result type";

bool IsWordBorder(char c) { return !std::isalnum(static_cast<unsigned char>(c)) && c != '"' && c != '_' && c != '-'; }

std::size_t QueryHash(const std::string& statement, const QueryParameters& params) {
    auto res = params.TypeHash();
    boost::hash_combine(res, std::hash<std::string>()(statement));
    return res;
}

class CountExecute {
public:
    CountExecute(Connection::Statistics& stats) : stats_(stats) {
        ++stats_.execute_total;
        exec_begin_time = SteadyClock::now();
    }

    ~CountExecute() {
        auto now = SteadyClock::now();
        if (!completed_) {
            ++stats_.error_execute_total;
        }
        stats_.sum_query_duration += now - exec_begin_time;
        stats_.last_execute_finish = now;
    }

    void AccountResult(ResultSet& result) {
        if (result.FieldCount()) ++stats_.reply_total;
        completed_ = true;
    }

private:
    Connection::Statistics& stats_;
    bool completed_{false};
    SteadyClock::time_point exec_begin_time;
};

class CountPortalBind {
public:
    CountPortalBind(Connection::Statistics& stats) : stats_(stats) { ++stats_.portal_bind_total; }

    ~CountPortalBind() {
        if (!completed_) ++stats_.error_execute_total;
    }

    void AccountResult(ResultSet&) { completed_ = true; }

private:
    Connection::Statistics& stats_;
    bool completed_{false};
};

struct TrackTrxEnd {
    TrackTrxEnd(Connection::Statistics& stats) : stats_(stats) {}
    ~TrackTrxEnd() { stats_.trx_end_time = SteadyClock::now(); }

    Connection::Statistics& Stats() { return stats_; }

private:
    Connection::Statistics& stats_;
};

struct CountCommit : TrackTrxEnd {
    CountCommit(Connection::Statistics& stats) : TrackTrxEnd(stats) { ++Stats().commit_total; }
};

struct CountRollback : TrackTrxEnd {
    CountRollback(Connection::Statistics& stats) : TrackTrxEnd(stats) { ++Stats().rollback_total; }
};

const std::string kSetLocalWorkMem = "SET LOCAL work_mem='256MB'";

const std::string kGetUserTypesSQL = R"~(
SELECT  t.oid,
        n.nspname,
        t.typname,
        t.typlen,
        t.typtype,
        t.typcategory,
        t.typrelid,
        t.typelem,
        t.typarray,
        t.typbasetype,
        t.typnotnull
FROM pg_catalog.pg_type t
  LEFT JOIN pg_catalog.pg_namespace n ON n.oid = t.typnamespace
  LEFT JOIN pg_catalog.pg_class c ON c.oid = t.typrelid
WHERE n.nspname NOT IN ('pg_catalog', 'pg_toast', 'information_schema')
  AND (c.relkind IS NULL OR c.relkind NOT IN ('i', 'S', 'I')))~";

const std::string kGetCompositeAttribsSQL = R"~(
SELECT c.reltype,
    a.attname,
    a.atttypid
FROM pg_catalog.pg_class c
LEFT JOIN pg_catalog.pg_namespace n ON n.oid = c.relnamespace
LEFT JOIN pg_catalog.pg_attribute a ON a.attrelid = c.oid
WHERE n.nspname NOT IN ('pg_catalog', 'pg_toast', 'information_schema')
  AND a.attnum > 0 AND NOT a.attisdropped
  AND c.relkind NOT IN ('i', 'S', 'I')
ORDER BY c.reltype, a.attnum)~";

const std::string kPingStatement = "SELECT 1 AS ping";

void CheckQueryParameters(const std::string& statement, const QueryParameters& params) {
    for (std::size_t i = 1; i <= params.Size(); ++i) {
        const auto arg_pos = statement.find(fmt::format("${}", i));

        UASSERT_MSG(
            !params.ParamBuffers()[i - 1] || arg_pos != std::string::npos,
            fmt::format("Parameter ${} is not null, but not used in query: '{}'", i, statement)
        );

        UINVARIANT(
            !params.ParamBuffers()[i - 1] || arg_pos != std::string::npos,
            fmt::format("Parameter ${} is not null, but not used in query", i)
        );
    }
}

constexpr std::string_view kCommands[] = {
    "select",
    "insert",
    "update",
    "with",
    "create",
    "alter",
    "begin",
    "commit",
    "rollback",
};

}  // namespace

std::string_view FindCommandName(std::string_view str) {
    const std::size_t max_search_depth = std::min(std::size_t{128}, str.size());
    const auto end_it = str.begin() + max_search_depth;

    const auto start = std::find_if_not(str.begin(), end_it, IsWordBorder);
    const auto end = std::find_if(start, end_it, IsWordBorder);
    const std::string_view command{start, static_cast<std::size_t>(end - start)};

    constexpr auto kKnownCommands = USERVER_NAMESPACE::utils::MakeTrivialSet<kCommands>();
    if (const auto match = kKnownCommands.GetIndexICase(command); match) {
        return kCommands[*match];
    }
    return {};
}

std::string FindQueryShortInfo(std::string_view prefix, std::string_view str) {
    if (const auto match = FindCommandName(str); !match.empty()) {
        return fmt::format("{}:{}", prefix, match);
    }

    return std::string{prefix};
}

struct ConnectionImpl::ResetTransactionCommandControl {
    ConnectionImpl& connection;

    ~ResetTransactionCommandControl() {
        connection.transaction_cmd_ctl_.reset();
        connection.current_statement_timeout_ =
            connection.testsuite_pg_ctl_.MakeStatementTimeout(connection.GetDefaultCommandControl().statement);
    }
};

ConnectionImpl::ConnectionImpl(
    engine::TaskProcessor& bg_task_processor,
    concurrent::BackgroundTaskStorageCore& bg_task_storage,
    uint32_t id,
    ConnectionSettings settings,
    const DefaultCommandControls& default_cmd_ctls,
    const testsuite::PostgresControl& testsuite_pg_ctl,
    const error_injection::Settings& ei_settings,
    engine::SemaphoreLock&& size_lock
)
    : uuid_{USERVER_NAMESPACE::utils::generators::GenerateUuid()},
      conn_wrapper_{bg_task_processor, bg_task_storage, id, std::move(size_lock)},
      prepared_{settings.max_prepared_cache_size},
      settings_{settings},
      default_cmd_ctls_(default_cmd_ctls),
      testsuite_pg_ctl_{testsuite_pg_ctl},
      ei_settings_(ei_settings) {
    if (settings_.max_prepared_cache_size == 0) {
        throw InvalidConfig("max_prepared_cache_size is 0");
    }
    if (settings_.max_ttl.has_value()) {
        // Actual ttl is randomized to avoid closing too many connections at the
        // same time
        auto ttl = (*settings_.max_ttl).count();
        ttl -= RandRange(ttl / 2);
        expires_at_ = SteadyNow() + std::chrono::seconds{ttl};
    }
#if !LIBPQ_HAS_PIPELINING
    if (settings_.pipeline_mode == PipelineMode::kEnabled) {
        LOG_LIMITED_WARNING() << "Pipeline mode is not supported, falling back";
        settings_.pipeline_mode = PipelineMode::kDisabled;
    }
#endif

    if (IsOmitDescribeInExecuteEnabled()) {
        LOG_DEBUG() << "Userver experiment pg-omit-describe-in-execute is enabled";
    }
}

void ConnectionImpl::AsyncConnect(const Dsn& dsn, engine::Deadline deadline) {
    tracing::Span span{scopes::kConnect};
    auto scope = span.CreateScopeTime();
    // While connecting there are several network roundtrips, so give them
    // some allowance.
    auto timeout = std::chrono::duration_cast<std::chrono::milliseconds>(deadline.TimeLeft());
    deadline = testsuite_pg_ctl_.MakeExecuteDeadline(timeout);
    conn_wrapper_.AsyncConnect(dsn, deadline, scope);
    conn_wrapper_.FillSpanTags(span, {timeout, GetStatementTimeout()});

    scope.Reset(scopes::kGetConnectData);
    // We cannot handle exceptions here, so we let them got to the caller
    if (settings_.discard_on_connect == ConnectionSettings::kDiscardAll) {
        ExecuteCommandNoPrepare("DISCARD ALL", deadline);
    }
    SetParameter("client_encoding", "UTF8", Connection::ParameterScope::kSession, deadline);
    RefreshReplicaState(deadline);
    SetConnectionStatementTimeout(GetDefaultCommandControl().statement, deadline);
    if (settings_.user_types != ConnectionSettings::kPredefinedTypesOnly) {
        LoadUserTypes(deadline);
    }
    if (settings_.pipeline_mode == PipelineMode::kEnabled) {
        conn_wrapper_.EnterPipelineMode();
    }
}

void ConnectionImpl::Close() { conn_wrapper_.Close().Wait(); }

int ConnectionImpl::GetServerVersion() const { return conn_wrapper_.GetServerVersion(); }

bool ConnectionImpl::IsInAbortedPipeline() const { return conn_wrapper_.IsInAbortedPipeline(); }

bool ConnectionImpl::IsInRecovery() const { return is_in_recovery_; }

bool ConnectionImpl::IsReadOnly() const { return is_read_only_; }

void ConnectionImpl::RefreshReplicaState(engine::Deadline deadline) {
    const auto in_hot_standby = conn_wrapper_.GetParameterStatus("in_hot_standby");
    const auto transaction_read_only = conn_wrapper_.GetParameterStatus("default_transaction_read_only");

    // Determine state without additional request, works with PostgreSQL >= 14.0
    if (!in_hot_standby.empty() && !transaction_read_only.empty()) {
        is_in_recovery_ = (in_hot_standby != "off");
        is_read_only_ = is_in_recovery_ || (transaction_read_only != "off");
        return;
    }

    const auto rows = ExecuteCommandNoPrepare(
        "SELECT pg_is_in_recovery(), "
        "current_setting('transaction_read_only')::bool",
        deadline
    );
    // Upon bugaevskiy's request
    // we are additionally checking two highly unlikely cases
    if (rows.IsEmpty()) {
        // 1. driver/PG protocol lost row
        LOG_WARNING() << "Cannot determine host recovery state, falling back to "
                         "read-only operation";
        is_in_recovery_ = true;
        is_read_only_ = true;
    } else {
        rows.Front().To(is_in_recovery_, is_read_only_);
        // 2. PG returns pg_is_in_recovery state which is inconsistent
        //    with transaction_read_only setting
        is_read_only_ = is_read_only_ || is_in_recovery_;
    }
}

ConnectionState ConnectionImpl::GetConnectionState() const { return conn_wrapper_.GetConnectionState(); }

bool ConnectionImpl::IsConnected() const { return GetConnectionState() > ConnectionState::kOffline; }

bool ConnectionImpl::IsIdle() const { return GetConnectionState() == ConnectionState::kIdle; }

bool ConnectionImpl::IsInTransaction() const { return GetConnectionState() > ConnectionState::kIdle; }

bool ConnectionImpl::IsPipelineActive() const { return conn_wrapper_.IsPipelineActive(); }

bool ConnectionImpl::ArePreparedStatementsEnabled() const {
    return settings_.prepared_statements != ConnectionSettings::kNoPreparedStatements;
}

bool ConnectionImpl::IsBroken() const { return conn_wrapper_.IsBroken(); }

bool ConnectionImpl::IsExpired() const { return expires_at_.has_value() && SteadyNow() > *expires_at_; }

ConnectionSettings const& ConnectionImpl::GetSettings() const { return settings_; }

CommandControl ConnectionImpl::GetDefaultCommandControl() const { return default_cmd_ctls_.GetDefaultCmdCtl(); }

void ConnectionImpl::UpdateDefaultCommandControl() {
    auto cmd_ctl = GetDefaultCommandControl();
    if (cmd_ctl != default_cmd_ctl_) {
        SetConnectionStatementTimeout(cmd_ctl.statement, testsuite_pg_ctl_.MakeExecuteDeadline(cmd_ctl.execute));
        default_cmd_ctl_ = cmd_ctl;
    }
}

const OptionalCommandControl& ConnectionImpl::GetTransactionCommandControl() const { return transaction_cmd_ctl_; }

OptionalCommandControl ConnectionImpl::GetNamedQueryCommandControl(const std::optional<Query::Name>& query_name) const {
    if (!query_name) return std::nullopt;
    return default_cmd_ctls_.GetQueryCmdCtl(query_name->GetUnderlying());
}

Connection::Statistics ConnectionImpl::GetStatsAndReset() {
    UASSERT_MSG(!IsInTransaction(), "GetStatsAndReset should be called outside of transaction");
    stats_.prepared_statements_current = prepared_.GetSize();
    return std::exchange(stats_, Connection::Statistics{});
}

ResultSet ConnectionImpl::ExecuteCommand(
    const Query& query,
    const QueryParameters& params,
    OptionalCommandControl statement_cmd_ctl
) {
    CheckBusy();

    auto pipeline_guard = std::optional<ScopeGuard>{};
    if (IsPipelineActive() && ICaseStartsWith(query.Statement(), kStatementVacuum)) {
        conn_wrapper_.ExitPipelineMode();
        pipeline_guard.emplace([this]() { conn_wrapper_.EnterPipelineMode(); });
    }

    auto deadline = testsuite_pg_ctl_.MakeExecuteDeadline(ExecuteTimeout(statement_cmd_ctl));
    SetStatementTimeout(std::move(statement_cmd_ctl));
    return ExecuteCommand(query, params, deadline);
}

void ConnectionImpl::Begin(
    const TransactionOptions& options,
    SteadyClock::time_point trx_start_time,
    OptionalCommandControl trx_cmd_ctl
) {
    if (IsInTransaction()) {
        throw AlreadyInTransaction();
    }
    DiscardOldPreparedStatements(MakeCurrentDeadline());
    stats_.trx_start_time = trx_start_time;
    stats_.work_start_time = SteadyClock::now();
    ++stats_.trx_total;
    if (IsPipelineActive()) {
        SendCommandNoPrepare(BeginStatement(options), MakeCurrentDeadline());
    } else {
        ExecuteCommandNoPrepare(BeginStatement(options), MakeCurrentDeadline());
    }
    if (trx_cmd_ctl) {
        SetTransactionCommandControl(*trx_cmd_ctl);
    }
}

void ConnectionImpl::Commit() {
    if (!IsInTransaction()) {
        throw NotInTransaction();
    }

    if (GetConnectionState() == ConnectionState::kTranError) {
        LOG_LIMITED_WARNING() << "Attempt to commit an aborted transaction, "
                                 "rollback will be performed instead";
        Rollback();
        throw RuntimeError{"Attempted to commit already failed transaction"};
    }

    CountCommit count_commit(stats_);
    ResetTransactionCommandControl transaction_guard{*this};
    ExecuteCommandNoPrepare("COMMIT", MakeCurrentDeadline());
}

void ConnectionImpl::Rollback() {
    if (!IsInTransaction()) {
        throw NotInTransaction();
    }
    if (IsBroken()) {
        throw RuntimeError{"Attempted to rollback a broken connection"};
    }

    CountRollback count_rollback(stats_);
    ResetTransactionCommandControl transaction_guard{*this};

    if (GetConnectionState() != ConnectionState::kTranActive ||
        (IsPipelineActive() && !conn_wrapper_.IsSyncingPipeline())) {
        ExecuteCommandNoPrepare("ROLLBACK", MakeCurrentDeadline());
    } else {
        LOG_DEBUG() << "Attempt to rollback transaction on a busy connection. "
                       "Probably a network error or a timeout happened";
    }
}

void ConnectionImpl::Start(SteadyClock::time_point start_time) {
    ++stats_.trx_total;
    ++stats_.out_of_trx;
    stats_.trx_start_time = start_time;
    stats_.work_start_time = SteadyClock::now();
    stats_.last_execute_finish = stats_.work_start_time;
}

void ConnectionImpl::Finish() { stats_.trx_end_time = SteadyClock::now(); }

Connection::StatementId ConnectionImpl::PortalBind(
    const std::string& statement,
    const std::string& portal_name,
    const QueryParameters& params,
    OptionalCommandControl statement_cmd_ctl
) {
    if (settings_.prepared_statements == ConnectionSettings::kNoPreparedStatements) {
        LOG_LIMITED_WARNING() << "Portals without prepared statements are currently unsupported";
        throw LogicError{"Prepared statements shouldn't be turned off while using portals"};
    }  // TODO Prepare unnamed query instead

    CheckBusy();
    TimeoutDuration network_timeout = ExecuteTimeout(statement_cmd_ctl);
    auto deadline = testsuite_pg_ctl_.MakeExecuteDeadline(network_timeout);
    SetStatementTimeout(std::move(statement_cmd_ctl));
    tracing::Span span{FindQueryShortInfo(scopes::kBind, statement)};
    conn_wrapper_.FillSpanTags(span, {network_timeout, GetStatementTimeout()});
    span.AddTag(tracing::kDatabaseStatement, statement);
    CheckDeadlineReached(deadline);
    auto scope = span.CreateScopeTime();
    CountPortalBind count_bind(stats_);

    const auto& prepared_info = DoPrepareStatement(statement, params, deadline, span, scope);

    scope.Reset(scopes::kBind);
    conn_wrapper_.SendPortalBind(prepared_info.statement_name, portal_name, params, scope);

    WaitResult(statement, deadline, network_timeout, count_bind, span, scope, nullptr);
    return prepared_info.id;
}

ResultSet ConnectionImpl::PortalExecute(
    Connection::StatementId statement_id,
    const std::string& portal_name,
    std::uint32_t n_rows,
    OptionalCommandControl statement_cmd_ctl
) {
    TimeoutDuration network_timeout = ExecuteTimeout(statement_cmd_ctl);

    auto deadline = testsuite_pg_ctl_.MakeExecuteDeadline(network_timeout);
    SetStatementTimeout(std::move(statement_cmd_ctl));

    auto* prepared_info = prepared_.Get(statement_id);
    UASSERT_MSG(
        prepared_info,
        "Portal execute uses statement id that is absent in prepared "
        "statements"
    );
    tracing::Span span{FindQueryShortInfo(scopes::kExec, prepared_info->statement)};
    conn_wrapper_.FillSpanTags(span, {network_timeout, GetStatementTimeout()});
    span.AddTag(tracing::kDatabaseStatement, prepared_info->statement);
    if (deadline.IsReached()) {
        ++stats_.execute_timeout;
        // TODO Portal name function, logging 'unnamed portal' for an empty name
        LOG_LIMITED_WARNING() << "Deadline was reached before starting to execute portal `" << portal_name << "`";
        throw ConnectionTimeoutError{"Deadline reached before executing"};
    }
    auto scope = span.CreateScopeTime(scopes::kExec);
    CountExecute count_execute(stats_);
    conn_wrapper_.SendPortalExecute(portal_name, n_rows, scope);

    return WaitResult(
        prepared_info->statement, deadline, network_timeout, count_execute, span, scope, &prepared_info->description
    );
}

void ConnectionImpl::Listen(std::string_view channel, OptionalCommandControl cmd_ctl) {
    ExecuteCommandNoPrepare(
        fmt::format(kStatementListen, conn_wrapper_.EscapeIdentifier(channel)),
        testsuite_pg_ctl_.MakeExecuteDeadline(ExecuteTimeout(cmd_ctl))
    );
}

void ConnectionImpl::Unlisten(std::string_view channel, OptionalCommandControl cmd_ctl) {
    ExecuteCommandNoPrepare(
        fmt::format(kStatementUnlisten, conn_wrapper_.EscapeIdentifier(channel)),
        testsuite_pg_ctl_.MakeExecuteDeadline(ExecuteTimeout(cmd_ctl))
    );
}

Notification ConnectionImpl::WaitNotify(engine::Deadline deadline) {
    CheckBusy();
    return conn_wrapper_.WaitNotify(deadline);
}

void ConnectionImpl::CancelAndCleanup(TimeoutDuration timeout) {
    auto deadline = testsuite_pg_ctl_.MakeExecuteDeadline(timeout);

    auto state = GetConnectionState();
    if (state == ConnectionState::kOffline) {
        return;
    }
    if (GetConnectionState() == ConnectionState::kTranActive) {
        auto cancel = conn_wrapper_.Cancel();
        // May throw on timeout
        try {
            conn_wrapper_.DiscardInput(deadline);
        } catch (const std::exception&) {
            // Consume error, we will throw later if we detect transaction is
            // busy
        }
        cancel.WaitUntil(deadline);
    }

    // DiscardInput could have marked connection as broken
    if (IsBroken()) {
        return;
    }

    // We might need more timeout here
    // We are no more bound with SLA, user has his exception.
    // It's better to keep this connection alive than recreating it, because
    // reconnecting impacts the pgbouncer badly
    Cleanup(timeout);
}

bool ConnectionImpl::Cleanup(TimeoutDuration timeout) {
    const auto deadline = testsuite_pg_ctl_.MakeExecuteDeadline(timeout);
    conn_wrapper_.DiscardInput(deadline);
    auto state = GetConnectionState();
    if (state == ConnectionState::kOffline) {
        return false;
    }
    if (state == ConnectionState::kTranActive) {
        return false;
    }
    // We might need more timeout here
    // We are no more bound with SLA, user has his exception.
    // We need to try and save the connection without canceling current query
    // not to kill the pgbouncer
    SetConnectionStatementTimeout(GetDefaultCommandControl().statement, deadline);
    if (IsPipelineActive()) {
        // In pipeline mode SetConnectionStatementTimeout writes a query into
        // connection query queue without waiting for its result.
        // We should process the results of this query, otherwise the connection
        // is not IDLE and gets deleted by the pool.
        //
        // If the query timeouts we won't be IDLE, and apart from timeouts there's
        // no other way for the query to fail, so just discard its result.
        conn_wrapper_.DiscardInput(deadline);
    } else if (settings_.pipeline_mode == PipelineMode::kEnabled) {
        // Reenter pipeline mode if necessary
        conn_wrapper_.EnterPipelineMode();
    }
    if (state > ConnectionState::kIdle) {
        Rollback();
    }
    return GetConnectionState() == ConnectionState::kIdle;
}

void ConnectionImpl::SetParameter(std::string_view name, std::string_view value, Connection::ParameterScope scope) {
    SetParameter(name, value, scope, MakeCurrentDeadline());
}

const UserTypes& ConnectionImpl::GetUserTypes() const { return db_types_; }

void ConnectionImpl::LoadUserTypes() { LoadUserTypes(MakeCurrentDeadline()); }

TimeoutDuration ConnectionImpl::GetIdleDuration() const { return conn_wrapper_.GetIdleDuration(); }

TimeoutDuration ConnectionImpl::GetStatementTimeout() const { return current_statement_timeout_; }

void ConnectionImpl::Ping() {
    Start(SteadyClock::now());
    ExecuteCommand(kPingStatement, MakeCurrentDeadline());
    Finish();
}

void ConnectionImpl::MarkAsBroken() { conn_wrapper_.MarkAsBroken(); }

void ConnectionImpl::CheckBusy() const {
    if ((GetConnectionState() == ConnectionState::kTranActive) &&
        (!IsPipelineActive() || conn_wrapper_.IsSyncingPipeline())) {
        throw ConnectionBusy("There is another query in flight");
    }
    if (IsInAbortedPipeline()) {
        throw ConnectionError("Attempted to use an aborted connection");
    }
}

void ConnectionImpl::CheckDeadlineReached(const engine::Deadline& deadline) {
    if (deadline.IsReached()) {
        ++stats_.execute_timeout;
        LOG_LIMITED_WARNING() << "Deadline was reached before starting to execute statement";
        throw ConnectionTimeoutError{"Deadline reached before executing"};
    }
}

tracing::Span ConnectionImpl::MakeQuerySpan(const Query& query, const CommandControl& cc) const {
    tracing::Span span{FindQueryShortInfo(scopes::kQuery, query.Statement())};
    conn_wrapper_.FillSpanTags(span, cc);
    query.FillSpanTags(span);
    return span;
}

engine::Deadline ConnectionImpl::MakeCurrentDeadline() const {
    return testsuite_pg_ctl_.MakeExecuteDeadline(CurrentExecuteTimeout());
}

void ConnectionImpl::SetTransactionCommandControl(CommandControl cmd_ctl) {
    if (!IsInTransaction()) {
        throw NotInTransaction{"Cannot set transaction command control out of transaction"};
    }
    transaction_cmd_ctl_ = cmd_ctl;
    SetStatementTimeout(cmd_ctl.statement, testsuite_pg_ctl_.MakeExecuteDeadline(cmd_ctl.execute));
}

TimeoutDuration ConnectionImpl::ExecuteTimeout(OptionalCommandControl cmd_ctl) const {
    if (!!cmd_ctl) {
        return cmd_ctl->execute;
    }
    return CurrentExecuteTimeout();
}

TimeoutDuration ConnectionImpl::CurrentExecuteTimeout() const {
    if (!!transaction_cmd_ctl_) {
        return transaction_cmd_ctl_->execute;
    }
    return GetDefaultCommandControl().execute;
}

void ConnectionImpl::SetConnectionStatementTimeout(TimeoutDuration timeout, engine::Deadline deadline) {
    timeout = testsuite_pg_ctl_.MakeStatementTimeout(timeout);
    if (current_statement_timeout_ != timeout) {
        SetParameter(
            kStatementTimeoutParameter, std::to_string(timeout.count()), Connection::ParameterScope::kSession, deadline
        );
        current_statement_timeout_ = timeout;
    }
}

void ConnectionImpl::SetStatementTimeout(TimeoutDuration timeout, engine::Deadline deadline) {
    timeout = testsuite_pg_ctl_.MakeStatementTimeout(timeout);
    if (current_statement_timeout_ != timeout) {
        SetParameter(
            kStatementTimeoutParameter,
            std::to_string(timeout.count()),
            Connection::ParameterScope::kTransaction,
            deadline
        );
        current_statement_timeout_ = timeout;
    }
}

void ConnectionImpl::SetStatementTimeout(OptionalCommandControl cmd_ctl) {
    if (!!cmd_ctl) {
        SetConnectionStatementTimeout(cmd_ctl->statement, testsuite_pg_ctl_.MakeExecuteDeadline(cmd_ctl->execute));
    } else if (!!transaction_cmd_ctl_) {
        SetStatementTimeout(
            transaction_cmd_ctl_->statement, testsuite_pg_ctl_.MakeExecuteDeadline(transaction_cmd_ctl_->execute)
        );
    } else {
        auto cmd_ctl = GetDefaultCommandControl();
        SetConnectionStatementTimeout(cmd_ctl.statement, testsuite_pg_ctl_.MakeExecuteDeadline(cmd_ctl.execute));
    }
}

const ConnectionImpl::PreparedStatementInfo& ConnectionImpl::DoPrepareStatement(
    const std::string& statement,
    const QueryParameters& params,
    engine::Deadline deadline,
    tracing::Span& span,
    tracing::ScopeTime& scope
) {
    auto query_hash = QueryHash(statement, params);
    Connection::StatementId query_id{query_hash};

    error_injection::Hook ei_hook(ei_settings_, deadline);
    ei_hook.PreHook<ConnectionTimeoutError, CommandError>();

    auto* statement_info = prepared_.Get(query_id);
    if (statement_info) {
        if (statement_info->description.pimpl_) {
            LOG_TRACE() << "Query " << statement << " is already prepared.";
            return *statement_info;
        } else {
            LOG_DEBUG() << "Found prepared but not described statement";
        }
    }

    if (prepared_.GetSize() >= settings_.max_prepared_cache_size) {
        auto statement_info = prepared_.GetLeastUsed();
        UASSERT(statement_info);
        DiscardPreparedStatement(*statement_info, deadline);
        prepared_.Erase(statement_info->id);
    }

    scope.Reset(scopes::kPrepare);
    LOG_TRACE() << "Query " << statement << " is not yet prepared";

    const std::string statement_name = "q" + std::to_string(query_hash) + "_" + uuid_;
    bool should_prepare = !statement_info;
    if (should_prepare) {
        conn_wrapper_.SendPrepare(statement_name, statement, params, scope);

        try {
            conn_wrapper_.WaitResult(deadline, scope, nullptr);
            LOG_DEBUG() << "Prepare successfully sent";
        } catch (const DuplicatePreparedStatement& e) {
            // As we have a pretty unique hash for a statement, we can safely use
            // it. This situation might happen when `SendPrepare` times out and we
            // erase the statement from `prepared_` map.
            LOG_DEBUG() << "Statement `" << statement
                        << "` was already prepared, there was possibly a timeout "
                           "while preparing, see log above.";
            ++stats_.duplicate_prepared_statements;

            // Mark query as already sent
            prepared_.Put(query_id, {query_id, statement, statement_name, ResultSet{nullptr}});

            if (IsInTransaction()) {
                // Transaction failed, need to throw
                throw;
            }
        } catch (const std::exception& e) {
            span.AddTag(tracing::kErrorFlag, true);
            throw;
        }
    } else {
        LOG_DEBUG() << "Don't send prepare, already sent";
    }

    conn_wrapper_.SendDescribePrepared(statement_name, scope);
    auto res = conn_wrapper_.WaitResult(deadline, scope, nullptr);
    if (!res.pimpl_) {
        throw CommandError("WaitResult() returned nullptr");
    }
    FillBufferCategories(res);
    // Ensure we've got binary format established
    res.GetRowDescription().CheckBinaryFormat(db_types_);

    if (!statement_info) {
        prepared_.Put(query_id, {query_id, statement, statement_name, std::move(res)});
        statement_info = prepared_.Get(query_id);
    } else {
        statement_info->description = std::move(res);
    }

    ++stats_.parse_total;

    return *statement_info;
}

void ConnectionImpl::DiscardOldPreparedStatements(engine::Deadline deadline) {
    // do not try to do anything in transaction as it may already be broken
    if (is_discard_prepared_pending_ && !IsInTransaction()) {
        LOG_DEBUG() << "Discarding prepared statements";
        prepared_.Clear();
        ExecuteCommandNoPrepare("DEALLOCATE ALL", deadline);
        is_discard_prepared_pending_ = false;
    }
}

void ConnectionImpl::DiscardPreparedStatement(const PreparedStatementInfo& info, engine::Deadline deadline) {
    LOG_DEBUG() << "Discarding prepared statement " << info.statement_name;
    ExecuteCommandNoPrepare("DEALLOCATE " + info.statement_name, deadline);
}

ResultSet ConnectionImpl::ExecuteCommand(const Query& query, engine::Deadline deadline) {
    static const QueryParameters kNoParams;
    return ExecuteCommand(query, kNoParams, deadline);
}

ResultSet ConnectionImpl::ExecuteCommand(const Query& query, const QueryParameters& params, engine::Deadline deadline) {
    if (settings_.prepared_statements == ConnectionSettings::kNoPreparedStatements) {
        return ExecuteCommandNoPrepare(query, params, deadline);
    }

    const auto& statement = query.Statement();
    if (settings_.ignore_unused_query_params == ConnectionSettings::kCheckUnused) {
        CheckQueryParameters(statement, params);
    }

    DiscardOldPreparedStatements(deadline);
    CheckDeadlineReached(deadline);
    const auto network_timeout = std::chrono::duration_cast<std::chrono::milliseconds>(deadline.TimeLeft());
    auto span = MakeQuerySpan(query, {network_timeout, GetStatementTimeout()});
    if (testsuite::AreTestpointsAvailable() && query.GetName()) {
        ReportStatement(query.GetName()->GetUnderlying());
    }

    auto scope = span.CreateScopeTime();
    CountExecute count_execute(stats_);

    auto const& prepared_info = DoPrepareStatement(statement, params, deadline, span, scope);

    const ResultSet* description_ptr_to_read = nullptr;
    PGresult* description_ptr_to_send = nullptr;
    if (IsOmitDescribeInExecuteEnabled()) {
        description_ptr_to_read = &prepared_info.description;
        description_ptr_to_send = description_ptr_to_read->pimpl_->handle_.get();
    }

    scope.Reset(scopes::kExec);
    conn_wrapper_.SendPreparedQuery(prepared_info.statement_name, params, scope, description_ptr_to_send);
    return WaitResult(statement, deadline, network_timeout, count_execute, span, scope, description_ptr_to_read);
}

const ConnectionImpl::PreparedStatementInfo&
ConnectionImpl::PrepareStatement(const Query& query, const detail::QueryParameters& params, TimeoutDuration timeout) {
    const auto deadline = testsuite_pg_ctl_.MakeExecuteDeadline(timeout);
    CheckDeadlineReached(deadline);

    const auto& statement = query.Statement();

    tracing::Span span{FindQueryShortInfo(scopes::kPrepare, query.Statement())};
    conn_wrapper_.FillSpanTags(span, {timeout, GetStatementTimeout()});
    span.AddTag(tracing::kDatabaseStatement, statement);

    auto scope = span.CreateScopeTime();
    return DoPrepareStatement(statement, params, deadline, span, scope);
}

void ConnectionImpl::AddIntoPipeline(
    CommandControl cc,
    const std::string& prepared_statement_name,
    const detail::QueryParameters& params,
    const ResultSet& description,
    tracing::ScopeTime& scope
) {
    // This is a precondition checked higher up the call stack.
    UASSERT(IsPipelineActive());
    // Sanity check, should never be hit.
    if (IsInAbortedPipeline()) {
        throw ConnectionError("Attempted to use an aborted connection");
    }

    SetStatementTimeout(cc);

    PGresult* description_to_send = IsOmitDescribeInExecuteEnabled() ? description.pimpl_->handle_.get() : nullptr;
    conn_wrapper_.SendPreparedQuery(prepared_statement_name, params, scope, description_to_send);

    conn_wrapper_.PutPipelineSync();
}

std::vector<ResultSet>
ConnectionImpl::GatherPipeline(TimeoutDuration timeout, const std::vector<ResultSet>& descriptions) {
    const auto deadline = testsuite_pg_ctl_.MakeExecuteDeadline(timeout);
    CheckDeadlineReached(deadline);

    std::vector<const PGresult*> native_descriptions(descriptions.size(), nullptr);
    if (IsOmitDescribeInExecuteEnabled()) {
        for (std::size_t i = 0; i < descriptions.size(); ++i) {
            native_descriptions[i] = descriptions[i].pimpl_->handle_.get();
        }
    }

    auto result = conn_wrapper_.GatherPipeline(deadline, native_descriptions);

    for (auto& single_result : result) {
        FillBufferCategories(single_result);
    }

    return result;
}

ResultSet ConnectionImpl::ExecuteCommandNoPrepare(const Query& query, engine::Deadline deadline) {
    static const QueryParameters kNoParams;
    return ExecuteCommandNoPrepare(query, kNoParams, deadline);
}

ResultSet
ConnectionImpl::ExecuteCommandNoPrepare(const Query& query, const QueryParameters& params, engine::Deadline deadline) {
    const auto& statement = query.Statement();
    auto network_timeout = std::chrono::duration_cast<std::chrono::milliseconds>(deadline.TimeLeft());
    CheckBusy();
    CheckDeadlineReached(deadline);
    auto span = MakeQuerySpan(query, {network_timeout, GetStatementTimeout()});
    auto scope = span.CreateScopeTime();
    CountExecute count_execute(stats_);
    conn_wrapper_.SendQuery(statement, params, scope);
    return WaitResult(statement, deadline, network_timeout, count_execute, span, scope, nullptr);
}

void ConnectionImpl::SendCommandNoPrepare(const Query& query, engine::Deadline deadline) {
    static const QueryParameters kNoParams;
    SendCommandNoPrepare(query, kNoParams, deadline);
}

void ConnectionImpl::SendCommandNoPrepare(
    const Query& query,
    const QueryParameters& params,
    engine::Deadline deadline
) {
    CheckBusy();
    CheckDeadlineReached(deadline);
    auto span = MakeQuerySpan(
        query, {std::chrono::duration_cast<std::chrono::milliseconds>(deadline.TimeLeft()), GetStatementTimeout()}
    );
    auto scope = span.CreateScopeTime();
    ++stats_.execute_total;
    conn_wrapper_.SendQuery(query.Statement(), params, scope);
}

void ConnectionImpl::SetParameter(
    std::string_view name,
    std::string_view value,
    Connection::ParameterScope scope,
    engine::Deadline deadline
) {
    bool is_transaction_scope = (scope == Connection::ParameterScope::kTransaction);
    LOG_DEBUG() << "Set '" << name << "' = '" << value << "' at " << (is_transaction_scope ? "transaction" : "session")
                << " scope";
    StaticQueryParameters<3> params;
    params.Write(db_types_, name, value, is_transaction_scope);
    if (IsPipelineActive()) {
        SendCommandNoPrepare(kSetConfigQuery, detail::QueryParameters{params}, deadline);
    } else {
        ExecuteCommand(kSetConfigQuery, detail::QueryParameters{params}, deadline);
    }
}

void ConnectionImpl::LoadUserTypes(engine::Deadline deadline) {
    UASSERT(settings_.user_types != ConnectionSettings::kPredefinedTypesOnly);
    try {
        // kSetLocalWorkMem help users with many user types to avoid
        // ConnectionInterrupted because there are `LEFT JOIN`s in queries
#if LIBPQ_HAS_PIPELINING
        conn_wrapper_.EnterPipelineMode();
        SendCommandNoPrepare("BEGIN", deadline);
        SendCommandNoPrepare(kSetLocalWorkMem, deadline);
#else
        ExecuteCommandNoPrepare("BEGIN", deadline);
        ExecuteCommandNoPrepare(kSetLocalWorkMem, deadline);
#endif
        auto types = ExecuteCommand(kGetUserTypesSQL, deadline).AsSetOf<DBTypeDescription>(kRowTag);
        auto attribs =
            ExecuteCommand(kGetCompositeAttribsSQL, deadline).AsContainer<UserTypes::CompositeFieldDefs>(kRowTag);
        ExecuteCommandNoPrepare("COMMIT", deadline);
#if LIBPQ_HAS_PIPELINING
        conn_wrapper_.ExitPipelineMode();
#else
#endif

        // End of definitions marker, to simplify processing
        attribs.push_back(CompositeFieldDef::EmptyDef());
        UserTypes db_types;
        for (auto desc : types) {
            db_types.AddType(std::move(desc));
        }
        db_types.AddCompositeFields(std::move(attribs));
        db_types_ = std::move(db_types);
    } catch (const Error& e) {
        LOG_LIMITED_ERROR() << "Error loading user datatypes: " << e;
        // TODO Decide about rethrowing
        throw;
    }
}

void ConnectionImpl::FillBufferCategories(ResultSet& res) {
    try {
        res.FillBufferCategories(db_types_);
    } catch (const UnknownBufferCategory& e) {
        if (settings_.user_types == ConnectionSettings::kPredefinedTypesOnly) {
            throw;
        }
        LOG_LIMITED_WARNING() << "Got a resultset with unknown datatype oid " << e.type_oid
                              << ". Will reload user datatypes";
        LoadUserTypes(MakeCurrentDeadline());
        // Don't catch the error again, let it fly to the user
        res.FillBufferCategories(db_types_);
    }
}

template <typename Counter>
ResultSet ConnectionImpl::WaitResult(
    const std::string& statement,
    engine::Deadline deadline,
    TimeoutDuration network_timeout,
    Counter& counter,
    tracing::Span& span,
    tracing::ScopeTime& scope,
    const ResultSet* description_ptr
) {
    const PGresult* description = description_ptr ? description_ptr->pimpl_->handle_.get() : nullptr;

    try {
        auto res = conn_wrapper_.WaitResult(deadline, scope, description);
        if (description_ptr) {
            res.SetBufferCategoriesFrom(*description_ptr);
        } else if (!res.IsEmpty()) {
            FillBufferCategories(res);
        }
        counter.AccountResult(res);
        return res;
    } catch (const InvalidSqlStatementName& e) {
        LOG_LIMITED_ERROR() << "Looks like your pg_bouncer is not in 'session' mode. "
                               "Please switch pg_bouncers's pooling mode to 'session'.";
        // reset prepared cache in case they just magically vanished
        is_discard_prepared_pending_ = true;
        span.AddTag(tracing::kErrorFlag, true);
        throw;
    } catch (const ConnectionTimeoutError& e) {
        ++stats_.execute_timeout;
        LOG_LIMITED_WARNING() << "Statement `" << statement << "` network timeout error: " << e << ". "
                              << "Network timeout was " << network_timeout.count() << "ms";
        span.AddTag(tracing::kErrorFlag, true);
        throw;
    } catch (const QueryCancelled& e) {
        ++stats_.execute_timeout;
        LOG_LIMITED_WARNING() << "Statement `" << statement << "` was cancelled: " << e << ". Statement timeout was "
                              << current_statement_timeout_.count() << "ms";
        span.AddTag(tracing::kErrorFlag, true);
        throw;
    } catch (const FeatureNotSupported& e) {
        // yes, this is the only way to discern this error
        if (e.GetServerMessage().GetPrimary() == kBadCachedPlanErrorMessage) {
            LOG_LIMITED_WARNING() << "Scheduling prepared statements invalidation due to "
                                     "cached plan change";
            is_discard_prepared_pending_ = true;
        }
        span.AddTag(tracing::kErrorFlag, true);
        throw;
    } catch (const std::exception&) {
        span.AddTag(tracing::kErrorFlag, true);
        throw;
    }
}

void ConnectionImpl::Cancel() { conn_wrapper_.Cancel().Wait(); }

void ConnectionImpl::ReportStatement(const std::string& name) {
    // Only report statement usage once.
    {
        std::unique_lock<engine::Mutex> lock{statements_mutex_};
        if (statements_reported_.count(name)) return;
    }

    try {
        TESTPOINT_CALLBACK("sql_statement", formats::json::MakeObject("name", name), ([&name, this](auto) {
                               std::unique_lock<engine::Mutex> lock{statements_mutex_};
                               statements_reported_.insert(name);
                           }));
    } catch (const std::exception& e) {
        LOG_WARNING() << e;
    }
}

bool ConnectionImpl::IsOmitDescribeInExecuteEnabled() const {
    return settings_.omit_describe_mode == OmitDescribeInExecuteMode::kEnabled;
}

}  // namespace storages::postgres::detail

USERVER_NAMESPACE_END
