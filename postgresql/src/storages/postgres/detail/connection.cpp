#include <storages/postgres/detail/connection.hpp>

#include <unordered_map>

#include <boost/functional/hash.hpp>

#include <libpq-fe.h>

#include <engine/io/socket.hpp>
#include <logging/log.hpp>
#include <utils/assert.hpp>
#include <utils/uuid4.hpp>

#include <engine/async.hpp>
#include <error_injection/hook.hpp>
#include <storages/postgres/detail/pg_connection_wrapper.hpp>
#include <storages/postgres/detail/result_wrapper.hpp>
#include <storages/postgres/detail/tracing_tags.hpp>
#include <storages/postgres/exceptions.hpp>
#include <storages/postgres/io/pg_type_parsers.hpp>
#include <storages/postgres/io/user_types.hpp>
#include <tracing/span.hpp>
#include <tracing/tags.hpp>

namespace storages {
namespace postgres {
namespace detail {

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

struct CountExecute {
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

  void AccountResult(const ResultSet& res, io::DataFormat format) {
    if (res.FieldCount()) {
      ++stats_.reply_total;
      if (format == io::DataFormat::kBinaryDataFormat) {
        ++stats_.bin_reply_total;
      }
    }
    completed_ = true;
  }

 private:
  Connection::Statistics& stats_;
  bool completed_ = false;
  SteadyClock::time_point exec_begin_time;
};

struct TrackTrxEnd {
  TrackTrxEnd(Connection::Statistics& stats) : stats_(stats) {}
  ~TrackTrxEnd() { stats_.trx_end_time = SteadyClock::now(); }

 protected:
  Connection::Statistics& stats_;
};

struct CountCommit : TrackTrxEnd {
  CountCommit(Connection::Statistics& stats) : TrackTrxEnd(stats) {
    ++stats_.commit_total;
  }
};

struct CountRollback : TrackTrxEnd {
  CountRollback(Connection::Statistics& stats) : TrackTrxEnd(stats) {
    ++stats_.rollback_total;
  }
};

const std::string kGetUserTypesSQL = R"~(
select  t.oid,
        n.nspname,
        t.typname,
        t.typlen,
        t.typtype,
        t.typcategory,
        t.typdelim,
        t.typrelid,
        t.typelem,
        t.typarray,
        t.typbasetype,
        t.typnotnull
from pg_catalog.pg_type t
  left join pg_catalog.pg_namespace n on n.oid = t.typnamespace
where n.nspname not in ('pg_catalog', 'pg_toast', 'information_schema')
order by t.oid)~";

const std::string kGetCompositeAttribsSQL = R"~(
select c.reltype,
    a.attname,
    a.atttypid
from pg_catalog.pg_class c
left join pg_catalog.pg_namespace n on n.oid = c.relnamespace
left join pg_catalog.pg_attribute a on a.attrelid = c.oid
where n.nspname not in ('pg_catalog', 'pg_toast', 'information_schema')
  and a.attnum > 0
order by c.reltype, a.attnum)~";

const std::string kPingStatement = "select 1 as ping";

const TimeoutDuration kConnectTimeout{2000};

}  // namespace

Connection::Statistics::Statistics() noexcept
    : trx_total(0),
      commit_total(0),
      rollback_total(0),
      out_of_trx(0),
      parse_total(0),
      execute_total(0),
      reply_total(0),
      bin_reply_total(0),
      error_execute_total(0),
      execute_timeout(0),
      duplicate_prepared_statements(0),
      sum_query_duration(0) {}

struct Connection::Impl {
  struct PreparedStatementInfo {
    StatementId id{};
    std::string statement;
    std::string statement_name;
    io::DataFormat format{io::DataFormat::kBinaryDataFormat};
    ResultSet description{nullptr};
  };
  using PreparedStatements =
      std::unordered_map<StatementId, PreparedStatementInfo>;

  Connection::Statistics stats_;
  PGConnectionWrapper conn_wrapper_;
  PreparedStatements prepared_;
  UserTypes db_types_;
  bool read_only_ = true;
  bool is_discard_prepared_pending_ = false;
  ConnectionSettings settings_;

  CommandControl default_cmd_ctl_;
  testsuite::PostgresControl testsuite_pg_ctl_;
  OptionalCommandControl transaction_cmd_ctl_;
  TimeoutDuration current_statement_timeout_{};
  const error_injection::Settings ei_settings_;

  std::string uuid_;

  Impl(engine::TaskProcessor& bg_task_processor, uint32_t id,
       ConnectionSettings settings, CommandControl default_cmd_ctl,
       const testsuite::PostgresControl& testsuite_pg_ctl,
       const error_injection::Settings& ei_settings, SizeGuard&& size_guard)
      : conn_wrapper_{bg_task_processor, id, std::move(size_guard)},
        settings_{settings},
        default_cmd_ctl_{default_cmd_ctl},
        testsuite_pg_ctl_{testsuite_pg_ctl},
        ei_settings_(ei_settings),
        uuid_{::utils::generators::GenerateUuid()} {}

  void Close() { conn_wrapper_.Close().Wait(); }

  void Cancel() { conn_wrapper_.Cancel().Wait(); }

  Connection::Statistics GetStatsAndReset() {
    UASSERT_MSG(!IsInTransaction(),
                "GetStatsAndReset should be called outside of transaction");
    return std::exchange(stats_, Connection::Statistics{});
  }

  void AsyncConnect(const std::string& conninfo) {
    tracing::Span span{scopes::kConnect};
    auto scope = span.CreateScopeTime();
    auto deadline = testsuite_pg_ctl_.MakeExecuteDeadline(kConnectTimeout);
    // While connecting there are several network roundtrips, so give them
    // some allowance.
    conn_wrapper_.AsyncConnect(conninfo, deadline, scope);
    conn_wrapper_.FillSpanTags(span);
    scope.Reset(scopes::kGetConnectData);
    // We cannot handle exceptions here, so we let them got to the caller
    ExecuteCommandNoPrepare("discard all", deadline);
    SetParameter("client_encoding", "UTF8", ParameterScope::kSession, deadline);
    CheckReadOnly(deadline);
    SetLocalTimezone(deadline);
    SetConnectionStatementTimeout(default_cmd_ctl_.statement, deadline);
    LoadUserTypes(deadline);
  }

  void CheckReadOnly(engine::Deadline deadline) {
    auto is_in_recovery =
        ExecuteCommandNoPrepare("SELECT pg_is_in_recovery()", deadline);
    if (!is_in_recovery.IsEmpty()) {
      is_in_recovery.Front().To(read_only_);
    } else {
      LOG_WARNING() << "Cannot determine host recovery state, falling back to "
                       "read-only operation";
      read_only_ = true;
    }

    if (!read_only_) {
      // Additional check for writability
      auto is_writable =
          ExecuteCommandNoPrepare("SHOW transaction_read_only", deadline);
      if (!is_writable.IsEmpty()) {
        is_writable.Front().To(read_only_);
        if (read_only_) {
          LOG_WARNING() << "Primary host is not writable, possibly due to "
                           "insufficient disk space";
        }
      }
    }
  }

  void DiscardOldPreparedStatements(engine::Deadline deadline) {
    // do not try to do anything in transaction as it may already be broken
    if (is_discard_prepared_pending_ && !IsInTransaction()) {
      LOG_DEBUG() << "Discarding prepared statements";
      prepared_.clear();
      ExecuteCommandNoPrepare("DEALLOCATE ALL", deadline);
      is_discard_prepared_pending_ = false;
    }
  }

  void SetDefaultCommandControl(const CommandControl& cmd_ctl) {
    if (cmd_ctl != default_cmd_ctl_) {
      SetConnectionStatementTimeout(
          cmd_ctl.statement,
          testsuite_pg_ctl_.MakeExecuteDeadline(cmd_ctl.execute));
      default_cmd_ctl_ = cmd_ctl;
    }
  }

  void SetTransactionCommandControl(CommandControl cmd_ctl) {
    if (!IsInTransaction()) {
      throw NotInTransaction{
          "Cannot set transaction command control out of transaction"};
    }
    transaction_cmd_ctl_ = cmd_ctl;
    SetStatementTimeout(cmd_ctl.statement,
                        testsuite_pg_ctl_.MakeExecuteDeadline(cmd_ctl.execute));
  }

  void ResetTransactionCommandControl() {
    transaction_cmd_ctl_.reset();
    current_statement_timeout_ =
        testsuite_pg_ctl_.MakeStatementTimeout(default_cmd_ctl_.statement);
  }

  TimeoutDuration CurrentExecuteTimeout() const {
    if (!!transaction_cmd_ctl_) {
      return transaction_cmd_ctl_->execute;
    }
    return default_cmd_ctl_.execute;
  }

  engine::Deadline MakeCurrentDeadline() const {
    return testsuite_pg_ctl_.MakeExecuteDeadline(CurrentExecuteTimeout());
  }

  /// @brief Set local timezone. If the timezone is invalid, catch the
  /// exception and log error.
  void SetLocalTimezone(engine::Deadline deadline) {
    const auto& tz = LocalTimezoneID();
    LOG_TRACE() << "Try and set pg timezone id " << tz.id << " canonical id "
                << tz.canonical_id;
    const auto& tz_id = tz.canonical_id.empty() ? tz.id : tz.canonical_id;
    try {
      SetParameter("TimeZone", tz_id, ParameterScope::kSession, deadline);
    } catch (const DataException&) {
      // No need to log the DataException message, it has been already logged
      // by connection wrapper.
      LOG_ERROR() << "Invalid value for time zone " << tz_id;
    }  // Let all other exceptions be propagated to the caller
  }

  void SetConnectionStatementTimeout(TimeoutDuration timeout,
                                     engine::Deadline deadline) {
    timeout = testsuite_pg_ctl_.MakeStatementTimeout(timeout);
    if (current_statement_timeout_ != timeout) {
      SetParameter(kStatementTimeoutParameter, std::to_string(timeout.count()),
                   ParameterScope::kSession, deadline);
      current_statement_timeout_ = timeout;
    }
  }

  void SetStatementTimeout(TimeoutDuration timeout, engine::Deadline deadline) {
    timeout = testsuite_pg_ctl_.MakeStatementTimeout(timeout);
    if (current_statement_timeout_ != timeout) {
      SetParameter(kStatementTimeoutParameter, std::to_string(timeout.count()),
                   ParameterScope::kTransaction, deadline);
      current_statement_timeout_ = timeout;
    }
  }

  void SetStatementTimeout(OptionalCommandControl cmd_ctl) {
    if (!!cmd_ctl) {
      SetConnectionStatementTimeout(
          cmd_ctl->statement,
          testsuite_pg_ctl_.MakeExecuteDeadline(cmd_ctl->execute));
    } else if (!!transaction_cmd_ctl_) {
      SetStatementTimeout(
          transaction_cmd_ctl_->statement,
          testsuite_pg_ctl_.MakeExecuteDeadline(transaction_cmd_ctl_->execute));
    } else {
      SetConnectionStatementTimeout(
          default_cmd_ctl_.statement,
          testsuite_pg_ctl_.MakeExecuteDeadline(default_cmd_ctl_.execute));
    }
  }

  void LoadUserTypes(engine::Deadline deadline) {
    try {
      auto types = ExecuteCommand(deadline, kGetUserTypesSQL)
                       .AsSetOf<DBTypeDescription>();
      auto attribs = ExecuteCommand(deadline, kGetCompositeAttribsSQL)
                         .AsContainer<UserTypes::CompositeFieldDefs>();
      // End of definitions marker, to simplify processing
      attribs.push_back(CompositeFieldDef::EmptyDef());
      UserTypes db_types;
      for (auto desc : types) {
        db_types.AddType(std::move(desc));
      }
      db_types.AddCompositeFields(std::move(attribs));
      db_types_ = std::move(db_types);
    } catch (const Error& e) {
      LOG_ERROR() << "Error loading user datatypes: " << e;
      // TODO Decide about rethrowing
      throw;
    }
  }

  ConnectionState GetConnectionState() const {
    return conn_wrapper_.GetConnectionState();
  }

  void SetParameter(const std::string& param, const std::string& value,
                    ParameterScope scope, engine::Deadline deadline) {
    bool is_transaction_scope = (scope == ParameterScope::kTransaction);
    auto log_level = (is_transaction_scope ? ::logging::Level::kDebug
                                           : ::logging::Level::kInfo);
    // TODO Log user/host/database
    LOG(log_level) << "Set '" << param << "' = '" << value << "' at "
                   << (is_transaction_scope ? "transaction" : "session")
                   << " scope";
    ExecuteCommand(deadline, "select set_config($1, $2, $3)", param, value,
                   is_transaction_scope);
  }

  template <typename... T>
  ResultSet ExecuteCommand(engine::Deadline deadline,
                           const std::string& statement, const T&... args) {
    detail::QueryParameters params;
    params.Write(db_types_, args...);
    return ExecuteCommand(statement, params, deadline);
  }

  void CheckBusy() {
    if (GetConnectionState() == ConnectionState::kTranActive) {
      throw ConnectionBusy("There is another query in flight");
    }
  }

  void FillBufferCategories(ResultSet& res) {
    try {
      res.FillBufferCategories(db_types_);
    } catch (const UnknownBufferCategory& e) {
      LOG_WARNING() << "Got a resultset with unknown datatype oid "
                    << e.type_oid << ". Will reload user datatypes";
      LoadUserTypes(MakeCurrentDeadline());
      // Don't catch the error again, let it fly to the user
      res.FillBufferCategories(db_types_);
    }
  }

  ResultSet ExecuteCommand(const std::string& statement,
                           const detail::QueryParameters& params,
                           OptionalCommandControl statement_cmd_ctl) {
    CheckBusy();
    TimeoutDuration execute_timeout = !!statement_cmd_ctl
                                          ? statement_cmd_ctl->execute
                                          : CurrentExecuteTimeout();
    auto deadline = testsuite_pg_ctl_.MakeExecuteDeadline(execute_timeout);
    SetStatementTimeout(std::move(statement_cmd_ctl));
    return ExecuteCommand(statement, params, deadline);
  }

  ResultSet ExecuteCommand(const std::string& statement,
                           const detail::QueryParameters& params,
                           engine::Deadline deadline,
                           OptionalCommandControl statement_cmd_ctl) {
    CheckBusy();
    if (!!statement_cmd_ctl &&
        deadline.TimeLeft() < statement_cmd_ctl->execute) {
      deadline =
          testsuite_pg_ctl_.MakeExecuteDeadline(statement_cmd_ctl->execute);
    }
    SetStatementTimeout(std::move(statement_cmd_ctl));
    return ExecuteCommand(statement, params, deadline);
  }

  const PreparedStatementInfo& PrepareStatement(
      const std::string& statement, const detail::QueryParameters& params,
      engine::Deadline deadline, tracing::Span& span, ScopeTime& scope) {
    auto query_hash = QueryHash(statement, params);
    StatementId query_id{query_hash};
    std::string statement_name = "q" + std::to_string(query_hash) + "_" + uuid_;

    error_injection::Hook ei_hook(ei_settings_, deadline);
    ei_hook.PreHook<ConnectionTimeoutError, CommandError>();

    if (prepared_.count(query_id)) {
      LOG_TRACE() << "Query " << statement << " is already prepared.";
      return prepared_[query_id];
    } else {
      scope.Reset(scopes::kPrepare);
      LOG_TRACE() << "Query " << statement << " is not yet prepared";
      conn_wrapper_.SendPrepare(statement_name, statement, params, scope);
      // Mark the statement prepared as soon as the send works correctly
      prepared_.insert(
          {query_id,
           {query_id, statement, statement_name,
            io::DataFormat::kBinaryDataFormat, ResultSet{nullptr}}});
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
        prepared_.erase(query_id);
        span.AddTag(tracing::kErrorFlag, true);
        throw;
      }

      conn_wrapper_.SendDescribePrepared(statement_name, scope);
      auto& info = prepared_[query_id];
      auto res = conn_wrapper_.WaitResult(deadline, scope);
      FillBufferCategories(res);
      info.description = res;
      // And now mark with actual protocol format
      try {
        res.GetRowDescription().CheckBinaryFormat(db_types_);
      } catch (const NoBinaryParser& e) {
        // TODO When text format will be deprecated in the driver, let the
        // exception fly to the client
        LOG_WARNING() << e;
        info.format = io::DataFormat::kTextDataFormat;
      }
      ++stats_.parse_total;
      return info;
    }
  }

  ResultSet ExecuteCommand(const std::string& statement,
                           const detail::QueryParameters& params,
                           engine::Deadline deadline) {
    if (settings_.prepared_statements ==
        ConnectionSettings::kNoPreparedStatements) {
      return ExecuteCommandNoPrepare(statement, params, deadline);
    }
    DiscardOldPreparedStatements(deadline);
    tracing::Span span{scopes::kQuery};
    conn_wrapper_.FillSpanTags(span);
    span.AddTag(tracing::kDatabaseStatement, statement);
    if (deadline.IsReached()) {
      ++stats_.execute_timeout;
      LOG_ERROR()
          << "Deadline was reached before starting to execute statement";
      throw ConnectionTimeoutError{"Deadline reached before executing"};
    }
    auto scope = span.CreateScopeTime();
    TimeoutDuration network_timeout =
        std::chrono::duration_cast<std::chrono::milliseconds>(
            deadline.TimeLeft());
    CountExecute count_execute(stats_);

    auto const& prepared_info =
        PrepareStatement(statement, params, deadline, span, scope);

    scope.Reset(scopes::kExec);
    conn_wrapper_.SendPreparedQuery(prepared_info.statement_name, params, scope,
                                    prepared_info.format);
    return WaitResult(statement, deadline, network_timeout, count_execute, span,
                      scope, prepared_info.format, prepared_info.description);
  }

  ResultSet ExecuteCommandNoPrepare(const std::string& statement) {
    return ExecuteCommandNoPrepare(
        statement,
        testsuite_pg_ctl_.MakeExecuteDeadline(CurrentExecuteTimeout()));
  }

  ResultSet ExecuteCommandNoPrepare(const std::string& statement,
                                    engine::Deadline deadline) {
    CheckBusy();
    tracing::Span span{scopes::kQuery};
    conn_wrapper_.FillSpanTags(span);
    span.AddTag(tracing::kDatabaseStatement, statement);
    CountExecute count_execute(stats_);
    TimeoutDuration network_timeout =
        std::chrono::duration_cast<std::chrono::milliseconds>(
            deadline.TimeLeft());

    auto scope = span.CreateScopeTime(scopes::kExec);
    conn_wrapper_.SendQuery(statement, scope);
    return WaitResult(statement, deadline, network_timeout, count_execute, span,
                      scope, io::DataFormat::kBinaryDataFormat,
                      ResultSet{nullptr});
  }

  ResultSet ExecuteCommandNoPrepare(const std::string& statement,
                                    const QueryParameters& params,
                                    engine::Deadline deadline) {
    CheckBusy();
    tracing::Span span{scopes::kQuery};
    conn_wrapper_.FillSpanTags(span);
    span.AddTag(tracing::kDatabaseStatement, statement);
    if (deadline.IsReached()) {
      ++stats_.execute_timeout;
      LOG_ERROR()
          << "Deadline was reached before starting to execute statement";
      throw ConnectionTimeoutError{"Deadline reached before executing"};
    }
    auto scope = span.CreateScopeTime();
    TimeoutDuration network_timeout =
        std::chrono::duration_cast<std::chrono::milliseconds>(
            deadline.TimeLeft());
    CountExecute count_execute(stats_);
    conn_wrapper_.SendQuery(statement, params, scope,
                            io::DataFormat::kBinaryDataFormat);
    return WaitResult(statement, deadline, network_timeout, count_execute, span,
                      scope, io::DataFormat::kBinaryDataFormat,
                      ResultSet{nullptr});
  }

  /// A separate method from ExecuteCommand as the method will be transformed
  /// to parse-bind-execute pipeline. This method is for experimenting with
  /// PostreSQL protocol and is intentionally separate from usual path method.
  ResultSet ExperimentalExecute(const std::string& statement,
                                io::DataFormat reply_format,
                                const detail::QueryParameters& params) {
    tracing::Span span{"pg_experimental_execute"};
    conn_wrapper_.FillSpanTags(span);
    span.AddTag(tracing::kDatabaseStatement, statement);
    auto scope = span.CreateScopeTime();
    conn_wrapper_.SendQuery(statement, params, scope, reply_format);
    auto res = conn_wrapper_.WaitResult(
        testsuite_pg_ctl_.MakeExecuteDeadline(CurrentExecuteTimeout()), scope);
    FillBufferCategories(res);
    return res;
  }

  StatementId PortalBind(const std::string& statement,
                         const std::string& portal_name,
                         const detail::QueryParameters& params,
                         OptionalCommandControl statement_cmd_ctl) {
    if (settings_.prepared_statements ==
        ConnectionSettings::kNoPreparedStatements) {
      LOG_ERROR()
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
    if (deadline.IsReached()) {
      ++stats_.execute_timeout;
      LOG_ERROR()
          << "Deadline was reached before starting to execute statement";
      throw ConnectionTimeoutError{"Deadline reached before executing"};
    }
    auto scope = span.CreateScopeTime();
    CountExecute count_execute(stats_);

    auto& prepared_info =
        PrepareStatement(statement, params, deadline, span, scope);

    scope.Reset(scopes::kBind);
    conn_wrapper_.SendPortalBind(prepared_info.statement_name, portal_name,
                                 params, scope, prepared_info.format);
    conn_wrapper_.ConsumeInput(deadline);
    return prepared_info.id;
  }

  ResultSet PortalExecute(StatementId statement_id,
                          const std::string& portal_name, std::uint32_t n_rows,
                          OptionalCommandControl statement_cmd_ctl) {
    TimeoutDuration network_timeout = !!statement_cmd_ctl
                                          ? statement_cmd_ctl->execute
                                          : CurrentExecuteTimeout();

    auto deadline = testsuite_pg_ctl_.MakeExecuteDeadline(network_timeout);
    SetStatementTimeout(std::move(statement_cmd_ctl));

    UASSERT_MSG(prepared_.count(statement_id),
                "Portal execute uses statement id that is absent in prepared "
                "statements");
    auto& prepared_info = prepared_[statement_id];

    tracing::Span span{scopes::kQuery};
    conn_wrapper_.FillSpanTags(span);
    span.AddTag(tracing::kDatabaseStatement, prepared_info.statement);
    if (deadline.IsReached()) {
      ++stats_.execute_timeout;
      // TODO Portal name function, logging 'unnamed portal' for an empty name
      LOG_ERROR() << "Deadline was reached before starting to execute portal `"
                  << portal_name << "`";
      throw ConnectionTimeoutError{"Deadline reached before executing"};
    }
    auto scope = span.CreateScopeTime(scopes::kExec);
    CountExecute count_execute(stats_);
    conn_wrapper_.SendPortalExecute(portal_name, n_rows, scope);

    return WaitResult(
        prepared_info.statement, deadline, network_timeout, count_execute, span,
        scope, io::DataFormat::kBinaryDataFormat, prepared_info.description);
  }

  ResultSet WaitResult(const std::string& statement, engine::Deadline deadline,
                       TimeoutDuration network_timeout,
                       CountExecute& count_execute, tracing::Span& span,
                       ScopeTime& scope, io::DataFormat fmt,
                       const ResultSet& description) {
    try {
      auto res = conn_wrapper_.WaitResult(deadline, scope);
      if (!description.IsEmpty()) {
        res.SetBufferCategoriesFrom(description);
      } else {
        FillBufferCategories(res);
      }
      count_execute.AccountResult(res, fmt);
      return res;
    } catch (const InvalidSqlStatementName& e) {
      LOG_ERROR() << "Looks like your pg_bouncer is not in 'session' mode. "
                     "Please switch pg_bouncers's pooling mode to 'session'. "
                     "Please see documentation here https://nda.ya.ru/3UXMpu";
      span.AddTag(tracing::kErrorFlag, true);
      throw;
    } catch (const ConnectionTimeoutError& e) {
      ++stats_.execute_timeout;
      LOG_ERROR() << "Statement `" << statement
                  << "` network timeout error: " << e << ". "
                  << "Network timout was " << network_timeout.count() << "ms";
      span.AddTag(tracing::kErrorFlag, true);
      throw;
    } catch (const QueryCanceled& e) {
      ++stats_.execute_timeout;
      LOG_ERROR() << "Statement `" << statement << "` was canceled: " << e
                  << ". Statement timeout was "
                  << current_statement_timeout_.count() << "ms";
      span.AddTag(tracing::kErrorFlag, true);
      throw;
    } catch (const FeatureNotSupported& e) {
      // yes, this is the only way to discern this error
      if (e.GetServerMessage().GetPrimary() == kBadCachedPlanErrorMessage) {
        LOG_WARNING() << "Scheduling prepared statements invalidation due to "
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

  void Begin(const TransactionOptions& options,
             SteadyClock::time_point&& trx_start_time,
             OptionalCommandControl trx_cmd_ctl = {}) {
    if (IsInTransaction()) {
      throw AlreadyInTransaction();
    }
    DiscardOldPreparedStatements(
        testsuite_pg_ctl_.MakeExecuteDeadline(CurrentExecuteTimeout()));
    stats_.trx_start_time = trx_start_time;
    stats_.work_start_time = SteadyClock::now();
    ++stats_.trx_total;
    ExecuteCommandNoPrepare(BeginStatement(options));
    if (trx_cmd_ctl) {
      SetTransactionCommandControl(*trx_cmd_ctl);
    }
  }

  void Commit() {
    if (!IsInTransaction()) {
      throw NotInTransaction();
    }
    CountCommit count_commit(stats_);
    ExecuteCommandNoPrepare("commit");
    ResetTransactionCommandControl();
  }

  void Rollback() {
    if (!IsInTransaction()) {
      throw NotInTransaction();
    }
    CountRollback count_rollback(stats_);
    ExecuteCommandNoPrepare("rollback");
    ResetTransactionCommandControl();
  }

  void Start(SteadyClock::time_point&& start_time) {
    ++stats_.trx_total;
    ++stats_.out_of_trx;
    stats_.trx_start_time = start_time;
    stats_.work_start_time = SteadyClock::now();
    stats_.last_execute_finish = stats_.work_start_time;
  }
  void Finish() { stats_.trx_end_time = SteadyClock::now(); }

  void CancelAndCleanup(TimeoutDuration timeout) {
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

  bool Cleanup(TimeoutDuration timeout) {
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
      SetConnectionStatementTimeout(default_cmd_ctl_.statement, deadline);
      return true;
    }
    return false;
  }

  bool IsConnected() const {
    return GetConnectionState() > ConnectionState::kOffline;
  }

  bool IsIdle() const { return GetConnectionState() == ConnectionState::kIdle; }

  bool IsInTransaction() const {
    return GetConnectionState() > ConnectionState::kIdle;
  }

  const UserTypes& GetUserTypes() const { return db_types_; }

  TimeoutDuration GetIdleDuration() const {
    return conn_wrapper_.GetIdleDuration();
  }

  void Ping() {
    Start(SteadyClock::now());
    ExecuteCommand(MakeCurrentDeadline(), kPingStatement);
    Finish();
  }
};  // Connection::Impl

std::unique_ptr<Connection> Connection::Connect(
    const std::string& conninfo, engine::TaskProcessor& bg_task_processor,
    uint32_t id, ConnectionSettings settings, CommandControl default_cmd_ctl,
    const testsuite::PostgresControl& testsuite_pg_ctl,
    const error_injection::Settings& ei_settings, SizeGuard&& size_guard) {
  std::unique_ptr<Connection> conn(new Connection());

  conn->pimpl_ = std::make_unique<Impl>(bg_task_processor, id, settings,
                                        default_cmd_ctl, testsuite_pg_ctl,
                                        ei_settings, std::move(size_guard));
  conn->pimpl_->AsyncConnect(conninfo);

  return conn;
}

Connection::Connection() = default;

Connection::~Connection() = default;

CommandControl Connection::GetDefaultCommandControl() const {
  return pimpl_->default_cmd_ctl_;
}

void Connection::SetDefaultCommandControl(const CommandControl& cmd_ctl) {
  pimpl_->SetDefaultCommandControl(cmd_ctl);
}

bool Connection::IsReadOnly() const { return pimpl_->read_only_; }
bool Connection::CheckReadOnly(engine::Deadline deadline) const {
  pimpl_->CheckReadOnly(deadline);
  return pimpl_->read_only_;
}

ConnectionState Connection::GetState() const {
  return pimpl_->GetConnectionState();
}

bool Connection::IsConnected() const { return pimpl_->IsConnected(); }

bool Connection::IsIdle() const { return pimpl_->IsIdle(); }

bool Connection::IsInTransaction() const { return pimpl_->IsInTransaction(); }

void Connection::Close() { pimpl_->Close(); }

Connection::Statistics Connection::GetStatsAndReset() {
  return pimpl_->GetStatsAndReset();
}

ResultSet Connection::Execute(const std::string& statement,
                              const detail::QueryParameters& params,
                              OptionalCommandControl statement_cmd_ctl) {
  return pimpl_->ExecuteCommand(statement, params,
                                std::move(statement_cmd_ctl));
}

ResultSet Connection::Execute(const std::string& statement,
                              const detail::QueryParameters& params,
                              engine::Deadline deadline,
                              OptionalCommandControl statement_cmd_ctl) {
  return pimpl_->ExecuteCommand(statement, params, deadline, statement_cmd_ctl);
}

void Connection::SetParameter(const std::string& param,
                              const std::string& value, ParameterScope scope) {
  pimpl_->SetParameter(param, value, scope, pimpl_->MakeCurrentDeadline());
}

void Connection::ReloadUserTypes() {
  pimpl_->LoadUserTypes(pimpl_->MakeCurrentDeadline());
}

const UserTypes& Connection::GetUserTypes() const {
  return pimpl_->GetUserTypes();
}

ResultSet Connection::ExperimentalExecute(
    const std::string& statement, io::DataFormat reply_format,
    const detail::QueryParameters& params) {
  return pimpl_->ExperimentalExecute(statement, reply_format, params);
}

Connection::StatementId Connection::PortalBind(
    const std::string& statement, const std::string& portal_name,
    const detail::QueryParameters& params,
    OptionalCommandControl statement_cmd_ctl) {
  return pimpl_->PortalBind(statement, portal_name, params,
                            std::move(statement_cmd_ctl));
}

ResultSet Connection::PortalExecute(StatementId statement_id,
                                    const std::string& portal_name,
                                    std::uint32_t n_rows,
                                    OptionalCommandControl statement_cmd_ctl) {
  return pimpl_->PortalExecute(statement_id, portal_name, n_rows,
                               std::move(statement_cmd_ctl));
}

void Connection::Begin(const TransactionOptions& options,
                       SteadyClock::time_point&& trx_start_time,
                       OptionalCommandControl trx_cmd_ctl) {
  // NOLINTNEXTLINE(hicpp-move-const-arg)
  pimpl_->Begin(options, std::move(trx_start_time), std::move(trx_cmd_ctl));
}

void Connection::Commit() { pimpl_->Commit(); }

void Connection::Rollback() { pimpl_->Rollback(); }

void Connection::Start(SteadyClock::time_point&& start_time) {
  // NOLINTNEXTLINE(hicpp-move-const-arg)
  pimpl_->Start(std::move(start_time));
}

void Connection::Finish() { pimpl_->Finish(); }

void Connection::CancelAndCleanup(TimeoutDuration timeout) {
  pimpl_->CancelAndCleanup(timeout);
}

bool Connection::Cleanup(TimeoutDuration timeout) {
  return pimpl_->Cleanup(timeout);
}

TimeoutDuration Connection::GetIdleDuration() const {
  return pimpl_->GetIdleDuration();
}

void Connection::Ping() { pimpl_->Ping(); }

}  // namespace detail
}  // namespace postgres
}  // namespace storages
