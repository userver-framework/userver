#include <storages/postgres/detail/connection_impl.hpp>

#include <boost/functional/hash.hpp>

#include <userver/error_injection/hook.hpp>
#include <userver/logging/log.hpp>
#include <userver/tracing/span.hpp>
#include <userver/tracing/tags.hpp>
#include <userver/utils/assert.hpp>
#include <userver/utils/uuid4.hpp>

#include <storages/postgres/detail/tracing_tags.hpp>
#include <storages/postgres/experiments.hpp>
#include <storages/postgres/io/pg_type_parsers.hpp>
#include <userver/storages/postgres/exceptions.hpp>

USERVER_NAMESPACE_BEGIN

namespace storages::postgres::detail {

namespace {

const char* const kStatementTimeoutParameter = "statement_timeout";

// we hope lc_messages is en_US, we don't control it anyway
const std::string kBadCachedPlanErrorMessage =
    "cached plan must not change result type";

std::size_t QueryHash(const std::string& statement,
                      const QueryParameters& params) {
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
  CountPortalBind(Connection::Statistics& stats) : stats_(stats) {
    ++stats_.portal_bind_total;
  }

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
  CountCommit(Connection::Statistics& stats) : TrackTrxEnd(stats) {
    ++Stats().commit_total;
  }
};

struct CountRollback : TrackTrxEnd {
  CountRollback(Connection::Statistics& stats) : TrackTrxEnd(stats) {
    ++Stats().rollback_total;
  }
};

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
  AND (c.relkind IS NULL OR c.relkind NOT IN ('i', 'S', 'I'))
ORDER BY t.oid)~";

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

void CheckQueryParameters(const std::string& statement,
                          const QueryParameters& params) {
  for (std::size_t i = 1; i <= params.Size(); ++i) {
    const auto arg_pos = statement.find(fmt::format("${}", i));

    UASSERT_MSG(
        !params.ParamBuffers()[i - 1] || arg_pos != std::string::npos,
        fmt::format("Parameter ${} is not null, but not used in query: '{}'", i,
                    statement));

    UINVARIANT(
        !params.ParamBuffers()[i - 1] || arg_pos != std::string::npos,
        fmt::format("Parameter ${} is not null, but not used in query", i));
  }
}

}  // namespace

struct ConnectionImpl::ResetTransactionCommandControl {
  ConnectionImpl& connection;

  ~ResetTransactionCommandControl() {
    connection.transaction_cmd_ctl_.reset();
    connection.current_statement_timeout_ =
        connection.testsuite_pg_ctl_.MakeStatementTimeout(
            connection.GetDefaultCommandControl().statement);
  }
};

ConnectionImpl::ConnectionImpl(
    engine::TaskProcessor& bg_task_processor,
    concurrent::BackgroundTaskStorageCore& bg_task_storage, uint32_t id,
    ConnectionSettings settings, const DefaultCommandControls& default_cmd_ctls,
    const testsuite::PostgresControl& testsuite_pg_ctl,
    const error_injection::Settings& ei_settings,
    engine::SemaphoreLock&& size_lock)
    : uuid_{USERVER_NAMESPACE::utils::generators::GenerateUuid()},
      conn_wrapper_{bg_task_processor, bg_task_storage, id,
                    std::move(size_lock)},
      prepared_{settings.max_prepared_cache_size},
      settings_{settings},
      default_cmd_ctls_(default_cmd_ctls),
      testsuite_pg_ctl_{testsuite_pg_ctl},
      ei_settings_(ei_settings) {
  if (settings_.max_prepared_cache_size == 0) {
    throw InvalidConfig("max_prepared_cache_size is 0");
  }
#if !LIBPQ_HAS_PIPELINING
  if (settings_.pipeline_mode == PipelineMode::kEnabled &&
      kPipelineExperiment.IsEnabled()) {
    LOG_LIMITED_WARNING() << "Pipeline mode is not supported, falling back";
    settings_.pipeline_mode = PipelineMode::kDisabled;
  }
#endif
}

void ConnectionImpl::AsyncConnect(const Dsn& dsn, engine::Deadline deadline) {
  tracing::Span span{scopes::kConnect};
  auto scope = span.CreateScopeTime();
  // While connecting there are several network roundtrips, so give them
  // some allowance.
  deadline = testsuite_pg_ctl_.MakeExecuteDeadline(
      std::chrono::duration_cast<std::chrono::milliseconds>(
          deadline.TimeLeft()));
  conn_wrapper_.AsyncConnect(dsn, deadline, scope);
  if (settings_.pipeline_mode == PipelineMode::kEnabled &&
      kPipelineExperiment.IsEnabled()) {
    conn_wrapper_.EnterPipelineMode();
  }
  conn_wrapper_.FillSpanTags(span);
  scope.Reset(scopes::kGetConnectData);
  // We cannot handle exceptions here, so we let them got to the caller
  ExecuteCommandNoPrepare("DISCARD ALL", deadline);
  SetParameter("client_encoding", "UTF8", Connection::ParameterScope::kSession,
               deadline);
  RefreshReplicaState(deadline);
  SetConnectionStatementTimeout(GetDefaultCommandControl().statement, deadline);
  if (settings_.user_types == ConnectionSettings::kUserTypesEnabled) {
    LoadUserTypes(deadline);
  }
}

void ConnectionImpl::Close() { conn_wrapper_.Close().Wait(); }

int ConnectionImpl::GetServerVersion() const {
  return conn_wrapper_.GetServerVersion();
}

bool ConnectionImpl::IsInRecovery() const { return is_in_recovery_; }

bool ConnectionImpl::IsReadOnly() const { return is_read_only_; }

void ConnectionImpl::RefreshReplicaState(engine::Deadline deadline) {
  // TODO: TAXICOMMON-4914
  // Something makes primary hosts report 'in_hot_standby' as 'on'.
  // Replaced with comparison below while under investigation.
  //
  // if (GetServerVersion() >= 140000) {
  //   const auto hot_standby_status =
  //       conn_wrapper_.GetParameterStatus("in_hot_standby");
  //   is_in_recovery_ = (hot_standby_status != "off");
  //   const auto txn_ro_status =
  //       conn_wrapper_.GetParameterStatus("default_transaction_read_only");
  //   is_read_only_ = is_in_recovery_ || (txn_ro_status != "off");
  //   return;
  // }

  const auto rows = ExecuteCommandNoPrepare(
      "SELECT pg_is_in_recovery(), "
      "current_setting('transaction_read_only')::bool",
      deadline);
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

  // TODO: TAXICOMMON-4914 (see above)
  if (GetServerVersion() >= 140000) {
    const auto hot_standby_status =
        conn_wrapper_.GetParameterStatus("in_hot_standby");
    const auto param_is_in_recovery = (hot_standby_status != "off");
    if (is_in_recovery_ != param_is_in_recovery) {
      LOG_LIMITED_INFO()
          << "Inconsistent replica state report: pg_is_in_recovery()="
          << is_in_recovery_ << " while in_hot_standby=" << hot_standby_status;
    }
    const auto txn_ro_status =
        conn_wrapper_.GetParameterStatus("default_transaction_read_only");
    const auto param_is_read_only = is_in_recovery_ || (txn_ro_status != "off");
    if (is_read_only_ != param_is_read_only) {
      LOG_LIMITED_INFO() << "Inconsistent replica state report: "
                            "current_setting('transaction_read_only')~"
                         << is_read_only_
                         << " while default_transaction_read_only~"
                         << param_is_read_only;
    }
  }
}

ConnectionState ConnectionImpl::GetConnectionState() const {
  return conn_wrapper_.GetConnectionState();
}

bool ConnectionImpl::IsConnected() const {
  return GetConnectionState() > ConnectionState::kOffline;
}

bool ConnectionImpl::IsIdle() const {
  return GetConnectionState() == ConnectionState::kIdle;
}

bool ConnectionImpl::IsInTransaction() const {
  return GetConnectionState() > ConnectionState::kIdle;
}

bool ConnectionImpl::IsPipelineActive() const {
  return conn_wrapper_.IsPipelineActive();
}

ConnectionSettings const& ConnectionImpl::GetSettings() const {
  return settings_;
}

CommandControl ConnectionImpl::GetDefaultCommandControl() const {
  return default_cmd_ctls_.GetDefaultCmdCtl();
}

void ConnectionImpl::UpdateDefaultCommandControl() {
  auto cmd_ctl = GetDefaultCommandControl();
  if (cmd_ctl != default_cmd_ctl_) {
    SetConnectionStatementTimeout(
        cmd_ctl.statement,
        testsuite_pg_ctl_.MakeExecuteDeadline(cmd_ctl.execute));
    default_cmd_ctl_ = cmd_ctl;
  }
}

const OptionalCommandControl& ConnectionImpl::GetTransactionCommandControl()
    const {
  return transaction_cmd_ctl_;
}

OptionalCommandControl ConnectionImpl::GetNamedQueryCommandControl(
    const std::optional<Query::Name>& query_name) const {
  if (!query_name) return std::nullopt;
  return default_cmd_ctls_.GetQueryCmdCtl(query_name->GetUnderlying());
}

Connection::Statistics ConnectionImpl::GetStatsAndReset() {
  UASSERT_MSG(!IsInTransaction(),
              "GetStatsAndReset should be called outside of transaction");
  stats_.prepared_statements_current = prepared_.GetSize();
  return std::exchange(stats_, Connection::Statistics{});
}

ResultSet ConnectionImpl::ExecuteCommand(
    const Query& query, const QueryParameters& params,
    OptionalCommandControl statement_cmd_ctl) {
  CheckBusy();
  TimeoutDuration execute_timeout = !!statement_cmd_ctl
                                        ? statement_cmd_ctl->execute
                                        : CurrentExecuteTimeout();
  auto deadline = testsuite_pg_ctl_.MakeExecuteDeadline(execute_timeout);
  SetStatementTimeout(std::move(statement_cmd_ctl));
  return ExecuteCommand(query, params, deadline);
}

void ConnectionImpl::Begin(const TransactionOptions& options,
                           SteadyClock::time_point trx_start_time,
                           OptionalCommandControl trx_cmd_ctl) {
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
  CountCommit count_commit(stats_);
  ResetTransactionCommandControl transaction_guard{*this};

  if (GetConnectionState() == ConnectionState::kTranError) {
    // TODO: TAXICOMMON-4103
    LOG_LIMITED_WARNING() << "Attempt to commit an aborted transaction, "
                             "rollback will be performed instead";
  }

  ExecuteCommandNoPrepare("COMMIT", MakeCurrentDeadline());
}

void ConnectionImpl::Rollback() {
  if (!IsInTransaction()) {
    throw NotInTransaction();
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
    const std::string& statement, const std::string& portal_name,
    const QueryParameters& params, OptionalCommandControl statement_cmd_ctl) {
  if (settings_.prepared_statements ==
      ConnectionSettings::kNoPreparedStatements) {
    LOG_LIMITED_WARNING()
        << "Portals without prepared statements are currently unsupported";
    throw LogicError{
        "Prepared statements shouldn't be turned off while using portals"};
  }  // TODO Prepare unnamed query instead

  CheckBusy();
  TimeoutDuration network_timeout = !!statement_cmd_ctl
                                        ? statement_cmd_ctl->execute
                                        : CurrentExecuteTimeout();
  auto deadline = testsuite_pg_ctl_.MakeExecuteDeadline(network_timeout);
  SetStatementTimeout(std::move(statement_cmd_ctl));

  tracing::Span span{scopes::kQuery};
  conn_wrapper_.FillSpanTags(span);
  span.AddTag(tracing::kDatabaseStatement, statement);
  CheckDeadlineReached(deadline);
  auto scope = span.CreateScopeTime();
  CountPortalBind count_bind(stats_);

  const auto& prepared_info =
      PrepareStatement(statement, params, deadline, span, scope);

  scope.Reset(scopes::kBind);
  conn_wrapper_.SendPortalBind(prepared_info.statement_name, portal_name,
                               params, scope);

  WaitResult(statement, deadline, network_timeout, count_bind, span, scope,
             nullptr);
  return prepared_info.id;
}

ResultSet ConnectionImpl::PortalExecute(
    Connection::StatementId statement_id, const std::string& portal_name,
    std::uint32_t n_rows, OptionalCommandControl statement_cmd_ctl) {
  TimeoutDuration network_timeout = !!statement_cmd_ctl
                                        ? statement_cmd_ctl->execute
                                        : CurrentExecuteTimeout();

  auto deadline = testsuite_pg_ctl_.MakeExecuteDeadline(network_timeout);
  SetStatementTimeout(std::move(statement_cmd_ctl));

  auto* prepared_info = prepared_.Get(statement_id);
  UASSERT_MSG(prepared_info,
              "Portal execute uses statement id that is absent in prepared "
              "statements");

  tracing::Span span{scopes::kQuery};
  conn_wrapper_.FillSpanTags(span);
  span.AddTag(tracing::kDatabaseStatement, prepared_info->statement);
  if (deadline.IsReached()) {
    ++stats_.execute_timeout;
    // TODO Portal name function, logging 'unnamed portal' for an empty name
    LOG_LIMITED_WARNING()
        << "Deadline was reached before starting to execute portal `"
        << portal_name << "`";
    throw ConnectionTimeoutError{"Deadline reached before executing"};
  }
  auto scope = span.CreateScopeTime(scopes::kExec);
  CountExecute count_execute(stats_);
  conn_wrapper_.SendPortalExecute(portal_name, n_rows, scope);

  return WaitResult(prepared_info->statement, deadline, network_timeout,
                    count_execute, span, scope, &prepared_info->description);
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

  // We might need more timeout here
  // We are no more bound with SLA, user has his exception.
  // It's better to keep this connection alive than recreating it, because
  // reconnecting impacts the pgbouncer badly
  Cleanup(timeout);
}

bool ConnectionImpl::Cleanup(TimeoutDuration timeout) {
  auto deadline = testsuite_pg_ctl_.MakeExecuteDeadline(timeout);
  if (conn_wrapper_.TryConsumeInput(deadline)) {
    auto state = GetConnectionState();
    if (state == ConnectionState::kOffline) {
      return false;
    }
    if (state == ConnectionState::kTranActive) {
      return false;
    }
    if (state > ConnectionState::kIdle) {
      Rollback();
    }
    // We might need more timeout here
    // We are no more bound with SLA, user has his exception.
    // We need to try and save the connection without canceling current query
    // not to kill the pgbouncer
    SetConnectionStatementTimeout(GetDefaultCommandControl().statement,
                                  deadline);
    return true;
  }
  return false;
}

void ConnectionImpl::SetParameter(std::string_view name, std::string_view value,
                                  Connection::ParameterScope scope) {
  SetParameter(name, value, scope, MakeCurrentDeadline());
}

const UserTypes& ConnectionImpl::GetUserTypes() const { return db_types_; }

void ConnectionImpl::LoadUserTypes() { LoadUserTypes(MakeCurrentDeadline()); }

TimeoutDuration ConnectionImpl::GetIdleDuration() const {
  return conn_wrapper_.GetIdleDuration();
}

TimeoutDuration ConnectionImpl::GetStatementTimeout() const {
  return current_statement_timeout_;
}

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
}

void ConnectionImpl::CheckDeadlineReached(const engine::Deadline& deadline) {
  if (deadline.IsReached()) {
    ++stats_.execute_timeout;
    LOG_LIMITED_WARNING()
        << "Deadline was reached before starting to execute statement";
    throw ConnectionTimeoutError{"Deadline reached before executing"};
  }
}

tracing::Span ConnectionImpl::MakeQuerySpan(const Query& query) const {
  tracing::Span span{scopes::kQuery};
  conn_wrapper_.FillSpanTags(span);
  query.FillSpanTags(span);
  return span;
}

engine::Deadline ConnectionImpl::MakeCurrentDeadline() const {
  return testsuite_pg_ctl_.MakeExecuteDeadline(CurrentExecuteTimeout());
}

void ConnectionImpl::SetTransactionCommandControl(CommandControl cmd_ctl) {
  if (!IsInTransaction()) {
    throw NotInTransaction{
        "Cannot set transaction command control out of transaction"};
  }
  transaction_cmd_ctl_ = cmd_ctl;
  SetStatementTimeout(cmd_ctl.statement,
                      testsuite_pg_ctl_.MakeExecuteDeadline(cmd_ctl.execute));
}

TimeoutDuration ConnectionImpl::CurrentExecuteTimeout() const {
  if (!!transaction_cmd_ctl_) {
    return transaction_cmd_ctl_->execute;
  }
  return GetDefaultCommandControl().execute;
}

void ConnectionImpl::SetConnectionStatementTimeout(TimeoutDuration timeout,
                                                   engine::Deadline deadline) {
  timeout = testsuite_pg_ctl_.MakeStatementTimeout(timeout);
  if (current_statement_timeout_ != timeout) {
    SetParameter(kStatementTimeoutParameter, std::to_string(timeout.count()),
                 Connection::ParameterScope::kSession, deadline);
    current_statement_timeout_ = timeout;
  }
}

void ConnectionImpl::SetStatementTimeout(TimeoutDuration timeout,
                                         engine::Deadline deadline) {
  timeout = testsuite_pg_ctl_.MakeStatementTimeout(timeout);
  if (current_statement_timeout_ != timeout) {
    SetParameter(kStatementTimeoutParameter, std::to_string(timeout.count()),
                 Connection::ParameterScope::kTransaction, deadline);
    current_statement_timeout_ = timeout;
  }
}

void ConnectionImpl::SetStatementTimeout(OptionalCommandControl cmd_ctl) {
  if (!!cmd_ctl) {
    SetConnectionStatementTimeout(
        cmd_ctl->statement,
        testsuite_pg_ctl_.MakeExecuteDeadline(cmd_ctl->execute));
  } else if (!!transaction_cmd_ctl_) {
    SetStatementTimeout(
        transaction_cmd_ctl_->statement,
        testsuite_pg_ctl_.MakeExecuteDeadline(transaction_cmd_ctl_->execute));
  } else {
    auto cmd_ctl = GetDefaultCommandControl();
    SetConnectionStatementTimeout(
        cmd_ctl.statement,
        testsuite_pg_ctl_.MakeExecuteDeadline(cmd_ctl.execute));
  }
}

const ConnectionImpl::PreparedStatementInfo& ConnectionImpl::PrepareStatement(
    const std::string& statement, const QueryParameters& params,
    engine::Deadline deadline, tracing::Span& span, tracing::ScopeTime& scope) {
  auto query_hash = QueryHash(statement, params);
  Connection::StatementId query_id{query_hash};
  std::string statement_name = "q" + std::to_string(query_hash) + "_" + uuid_;

  error_injection::Hook ei_hook(ei_settings_, deadline);
  ei_hook.PreHook<ConnectionTimeoutError, CommandError>();

  auto* statement_info = prepared_.Get(query_id);
  if (statement_info) {
    LOG_TRACE() << "Query " << statement << " is already prepared.";
    return *statement_info;
  } else {
    if (prepared_.GetSize() >= settings_.max_prepared_cache_size) {
      statement_info = prepared_.GetLeastUsed();
      UASSERT(statement_info);
      DiscardPreparedStatement(*statement_info, deadline);
      prepared_.Erase(statement_info->id);
    }
    scope.Reset(scopes::kPrepare);
    LOG_TRACE() << "Query " << statement << " is not yet prepared";
    conn_wrapper_.SendPrepare(statement_name, statement, params, scope);
    // Mark the statement prepared as soon as the send works correctly
    prepared_.Put(query_id,
                  {query_id, statement, statement_name, ResultSet{nullptr}});
    try {
      conn_wrapper_.WaitResult(deadline, scope);
    } catch (const DuplicatePreparedStatement& e) {
      // As we have a pretty unique hash for a statement, we can safely use
      // it. This situation might happen when `SendPrepare` times out and we
      // erase the statement from `prepared_` map.
      LOG_DEBUG() << "Statement `" << statement
                  << "` was already prepared, there was possibly a timeout "
                     "while preparing, see log above.";
      ++stats_.duplicate_prepared_statements;
    } catch (const std::exception&) {
      prepared_.Erase(query_id);
      span.AddTag(tracing::kErrorFlag, true);
      throw;
    }

    conn_wrapper_.SendDescribePrepared(statement_name, scope);
    statement_info = prepared_.Get(query_id);
    auto res = conn_wrapper_.WaitResult(deadline, scope);
    if (!res.pimpl_) {
      throw CommandError("WaitResult() returned nullptr");
    }
    FillBufferCategories(res);
    statement_info->description = res;
    // Ensure we've got binary format established
    res.GetRowDescription().CheckBinaryFormat(db_types_);
    ++stats_.parse_total;
    return *statement_info;
  }
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

void ConnectionImpl::DiscardPreparedStatement(const PreparedStatementInfo& info,
                                              engine::Deadline deadline) {
  LOG_DEBUG() << "Discarding prepared statement " << info.statement_name;
  ExecuteCommandNoPrepare("DEALLOCATE " + info.statement_name, deadline);
}

ResultSet ConnectionImpl::ExecuteCommand(const Query& query,
                                         engine::Deadline deadline) {
  static const QueryParameters kNoParams;
  return ExecuteCommand(query, kNoParams, deadline);
}

ResultSet ConnectionImpl::ExecuteCommand(const Query& query,
                                         const QueryParameters& params,
                                         engine::Deadline deadline) {
  if (settings_.prepared_statements ==
      ConnectionSettings::kNoPreparedStatements) {
    return ExecuteCommandNoPrepare(query, params, deadline);
  }

  const auto& statement = query.Statement();
  if (settings_.ignore_unused_query_params ==
      ConnectionSettings::kCheckUnused) {
    CheckQueryParameters(statement, params);
  }

  DiscardOldPreparedStatements(deadline);
  CheckDeadlineReached(deadline);
  auto span = MakeQuerySpan(query);
  auto scope = span.CreateScopeTime();
  TimeoutDuration network_timeout =
      std::chrono::duration_cast<std::chrono::milliseconds>(
          deadline.TimeLeft());
  CountExecute count_execute(stats_);

  auto const& prepared_info =
      PrepareStatement(statement, params, deadline, span, scope);

  scope.Reset(scopes::kExec);
  conn_wrapper_.SendPreparedQuery(prepared_info.statement_name, params, scope);
  return WaitResult(statement, deadline, network_timeout, count_execute, span,
                    scope, &prepared_info.description);
}

ResultSet ConnectionImpl::ExecuteCommandNoPrepare(const Query& query,
                                                  engine::Deadline deadline) {
  static const QueryParameters kNoParams;
  return ExecuteCommandNoPrepare(query, kNoParams, deadline);
}

ResultSet ConnectionImpl::ExecuteCommandNoPrepare(const Query& query,
                                                  const QueryParameters& params,
                                                  engine::Deadline deadline) {
  const auto& statement = query.Statement();
  CheckBusy();
  CheckDeadlineReached(deadline);
  auto span = MakeQuerySpan(query);
  auto scope = span.CreateScopeTime();
  TimeoutDuration network_timeout =
      std::chrono::duration_cast<std::chrono::milliseconds>(
          deadline.TimeLeft());
  CountExecute count_execute(stats_);
  conn_wrapper_.SendQuery(statement, params, scope);
  return WaitResult(statement, deadline, network_timeout, count_execute, span,
                    scope, nullptr);
}

void ConnectionImpl::SendCommandNoPrepare(const Query& query,
                                          engine::Deadline deadline) {
  static const QueryParameters kNoParams;
  SendCommandNoPrepare(query, kNoParams, deadline);
}

void ConnectionImpl::SendCommandNoPrepare(const Query& query,
                                          const QueryParameters& params,
                                          engine::Deadline deadline) {
  CheckBusy();
  CheckDeadlineReached(deadline);
  auto span = MakeQuerySpan(query);
  auto scope = span.CreateScopeTime();
  ++stats_.execute_total;
  conn_wrapper_.SendQuery(query.Statement(), params, scope);
}

void ConnectionImpl::SetParameter(std::string_view name, std::string_view value,
                                  Connection::ParameterScope scope,
                                  engine::Deadline deadline) {
  bool is_transaction_scope =
      (scope == Connection::ParameterScope::kTransaction);
  LOG_DEBUG() << "Set '" << name << "' = '" << value << "' at "
              << (is_transaction_scope ? "transaction" : "session") << " scope";
  StaticQueryParameters<3> params;
  params.Write(db_types_, name, value, is_transaction_scope);
  if (IsPipelineActive() && IsInTransaction()) {
    SendCommandNoPrepare("SELECT set_config($1, $2, $3)",
                         detail::QueryParameters{params}, deadline);
  } else {
    ExecuteCommand("SELECT set_config($1, $2, $3)",
                   detail::QueryParameters{params}, deadline);
  }
}

void ConnectionImpl::LoadUserTypes(engine::Deadline deadline) {
  UASSERT(settings_.user_types == ConnectionSettings::kUserTypesEnabled);
  try {
    auto types = ExecuteCommand(kGetUserTypesSQL, deadline)
                     .AsSetOf<DBTypeDescription>(kRowTag);
    auto attribs = ExecuteCommand(kGetCompositeAttribsSQL, deadline)
                       .AsContainer<UserTypes::CompositeFieldDefs>(kRowTag);
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
    LOG_LIMITED_WARNING() << "Got a resultset with unknown datatype oid "
                          << e.type_oid << ". Will reload user datatypes";
    LoadUserTypes(MakeCurrentDeadline());
    // Don't catch the error again, let it fly to the user
    res.FillBufferCategories(db_types_);
  }
}

template <typename Counter>
ResultSet ConnectionImpl::WaitResult(const std::string& statement,
                                     engine::Deadline deadline,
                                     TimeoutDuration network_timeout,
                                     Counter& counter, tracing::Span& span,
                                     tracing::ScopeTime& scope,
                                     const ResultSet* description_ptr) {
  try {
    auto res = conn_wrapper_.WaitResult(deadline, scope);
    if (description_ptr && !description_ptr->IsEmpty()) {
      res.SetBufferCategoriesFrom(*description_ptr);
    } else if (!res.IsEmpty()) {
      FillBufferCategories(res);
    }
    counter.AccountResult(res);
    return res;
  } catch (const InvalidSqlStatementName& e) {
    LOG_LIMITED_ERROR()
        << "Looks like your pg_bouncer is not in 'session' mode. "
           "Please switch pg_bouncers's pooling mode to 'session'.";
    // reset prepared cache in case they just magically vanished
    is_discard_prepared_pending_ = true;
    span.AddTag(tracing::kErrorFlag, true);
    throw;
  } catch (const ConnectionTimeoutError& e) {
    ++stats_.execute_timeout;
    LOG_LIMITED_WARNING() << "Statement `" << statement
                          << "` network timeout error: " << e << ". "
                          << "Network timeout was " << network_timeout.count()
                          << "ms";
    span.AddTag(tracing::kErrorFlag, true);
    throw;
  } catch (const QueryCancelled& e) {
    ++stats_.execute_timeout;
    LOG_LIMITED_WARNING() << "Statement `" << statement
                          << "` was cancelled: " << e
                          << ". Statement timeout was "
                          << current_statement_timeout_.count() << "ms";
    span.AddTag(tracing::kErrorFlag, true);
    throw;
  } catch (const FeatureNotSupported& e) {
    // yes, this is the only way to discern this error
    if (e.GetServerMessage().GetPrimary() == kBadCachedPlanErrorMessage) {
      LOG_LIMITED_WARNING()
          << "Scheduling prepared statements invalidation due to "
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

}  // namespace storages::postgres::detail

USERVER_NAMESPACE_END
