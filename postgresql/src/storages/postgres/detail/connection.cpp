#include <storages/postgres/detail/connection.hpp>

#include <unordered_map>

#include <boost/functional/hash.hpp>

#include <libpq-fe.h>

#include <engine/io/socket.hpp>
#include <logging/log.hpp>

#include <engine/async.hpp>
#include <storages/postgres/detail/pg_connection_wrapper.hpp>
#include <storages/postgres/detail/result_wrapper.hpp>
#include <storages/postgres/exceptions.hpp>
#include <storages/postgres/io/pg_type_parsers.hpp>
#include <storages/postgres/io/user_types.hpp>

namespace storages {
namespace postgres {
namespace detail {

namespace {

const char* const kStatementTimeoutParameter = "statement_timeout";

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
    if (!completed_) {
      ++stats_.error_execute_total;
    }
    stats_.sum_query_duration += SteadyClock::now() - exec_begin_time;
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

const TimeoutType kConnectTimeout{2000};

}  // namespace

Connection::Statistics::Statistics() noexcept
    : trx_total(0),
      commit_total(0),
      rollback_total(0),
      parse_total(0),
      execute_total(0),
      reply_total(0),
      bin_reply_total(0),
      error_execute_total(0),
      sum_query_duration(0) {}

struct Connection::Impl {
  using PreparedStatements = std::unordered_map<std::size_t, io::DataFormat>;

  Connection::Statistics stats_;
  PGConnectionWrapper conn_wrapper_;
  PreparedStatements prepared_;
  UserTypes db_types_;
  bool read_only_ = true;

  CommandControl default_cmd_ctl_;
  OptionalCommandControl transaction_cmd_ctl_;
  TimeoutType current_statement_timeout_;

  struct StatementTimeoutCentry {
    StatementTimeoutCentry(Impl* conn, OptionalCommandControl cmd_ctl)
        : connection_{conn},
          cmd_ctl_{std::move(cmd_ctl)},
          old_timeout_{conn->current_statement_timeout_},
          session_scope_{false} {
      if (cmd_ctl_.is_initialized()) {
        if (connection_->IsInTransaction()) {
          connection_->SetStatementTimeout(cmd_ctl_->statement);
        } else {
          connection_->SetDefaultStatementTimeout(cmd_ctl_->statement);
          session_scope_ = true;
        }
      }
    }

    ~StatementTimeoutCentry() noexcept {
      if (cmd_ctl_.is_initialized() &&
          connection_->current_statement_timeout_ != old_timeout_ &&
          connection_->GetConnectionState() != ConnectionState::kTranActive) {
        try {
          if (session_scope_) {
            connection_->SetDefaultStatementTimeout(old_timeout_);
          } else {
            connection_->SetStatementTimeout(old_timeout_);
          }
        } catch (std::exception const& e) {
          LOG_ERROR() << "Failed to reset old statement timeout: " << e.what();
        }
      }
    }

   private:
    Impl* const connection_;
    OptionalCommandControl cmd_ctl_;
    TimeoutType old_timeout_;
    bool session_scope_;
  };

  Impl(engine::TaskProcessor& bg_task_processor, uint32_t id,
       CommandControl default_cmd_ctl)
      : conn_wrapper_{bg_task_processor, id},
        default_cmd_ctl_{default_cmd_ctl} {}

  void Close() { conn_wrapper_.Close().Wait(); }

  void Cancel() { conn_wrapper_.Cancel().Wait(); }

  Connection::Statistics GetStatsAndReset() {
    assert(!IsInTransaction() &&
           "GetStatsAndReset should be called outside of transaction");
    return std::exchange(stats_, Connection::Statistics{});
  }

  // TODO Add tracing::Span
  void AsyncConnect(const std::string& conninfo) {
    // While connecting there are several network roundtrips, so give them
    // some allowance.
    conn_wrapper_.AsyncConnect(conninfo, kConnectTimeout);
    // We cannot handle exceptions here, so we let them got to the caller
    // Detect if the connection is read only.
    auto res = ExecuteCommandNoPrepare("show transaction_read_only");
    if (!res.IsEmpty()) {
      res.Front().To(read_only_);
    }
    SetLocalTimezone();
    SetDefaultStatementTimeout(default_cmd_ctl_.statement);
    LoadUserTypes();
  }

  void SetDefaultCommandControl(CommandControl cmd_ctl) {
    SetDefaultStatementTimeout(cmd_ctl.statement);
    default_cmd_ctl_ = cmd_ctl;
  }

  void SetTransactionCommandControl(CommandControl cmd_ctl) {
    if (!IsInTransaction()) {
      throw NotInTransaction{
          "Cannot set transaction command control out of transaction"};
    }
    transaction_cmd_ctl_ = cmd_ctl;
    SetStatementTimeout(cmd_ctl.statement);
  }

  void ResetTransactionCommandControl() {
    transaction_cmd_ctl_.reset();
    current_statement_timeout_ = default_cmd_ctl_.statement;
  }

  TimeoutType CurrentNetworkTimeout() const {
    if (transaction_cmd_ctl_.is_initialized()) {
      return transaction_cmd_ctl_->network;
    }
    return default_cmd_ctl_.network;
  }

  TimeoutType CurrentStatementTimeout() const {
    if (transaction_cmd_ctl_.is_initialized()) {
      return transaction_cmd_ctl_->statement;
    }
    return default_cmd_ctl_.statement;
  }

  /// @brief Set local timezone. If the timezone is invalid, catch the
  /// exception and log error.
  void SetLocalTimezone() {
    const auto& tz = LocalTimezoneID();
    LOG_TRACE() << "Try and set pg timezone id " << tz.id << " canonical id "
                << tz.canonical_id;
    const auto& tz_id = tz.canonical_id.empty() ? tz.id : tz.canonical_id;
    try {
      SetParameter("TimeZone", tz_id, ParameterScope::kSession);
    } catch (const DataException&) {
      // No need to log the DataException message, it has been already logged
      // by connection wrapper.
      LOG_ERROR() << "Invalid value for time zone " << tz_id;
    }  // Let all other exceptions be propagated to the caller
  }

  void SetDefaultStatementTimeout(TimeoutType timeout) {
    if (current_statement_timeout_ != timeout) {
      SetParameter(kStatementTimeoutParameter, std::to_string(timeout.count()),
                   ParameterScope::kSession);
      current_statement_timeout_ = timeout;
    }
  }

  void SetStatementTimeout(TimeoutType timeout) {
    if (current_statement_timeout_ != timeout) {
      SetParameter(kStatementTimeoutParameter, std::to_string(timeout.count()),
                   ParameterScope::kTransaction);
      current_statement_timeout_ = timeout;
    }
  }

  void LoadUserTypes() {
    try {
      auto types =
          ExecuteCommand(kGetUserTypesSQL).AsSetOf<DBTypeDescription>();
      auto attribs = ExecuteCommand(kGetCompositeAttribsSQL)
                         .AsContainer<UserTypes::CompositeFieldDefs>();
      // End of definitions marker, to simplify processing
      attribs.push_back(CompositeFieldDef::EmptyDef());
      db_types_.Reset();
      for (auto desc : types) {
        db_types_.AddType(std::move(desc));
      }
      db_types_.AddCompositeFields(std::move(attribs));
    } catch (const Error& e) {
      LOG_ERROR() << "Error loading user datatypes: " << e.what();
      // TODO Decide about rethrowing
      throw;
    }
  }

  const logging::LogExtra& GetLogExtra() const {
    return conn_wrapper_.GetLogExtra();
  }

  ConnectionState GetConnectionState() const {
    return conn_wrapper_.GetConnectionState();
  }

  void SetParameter(const std::string& param, const std::string& value,
                    ParameterScope scope) {
    bool is_transaction_scope = (scope == ParameterScope::kTransaction);
    auto log_level = (is_transaction_scope ? ::logging::Level::kDebug
                                           : ::logging::Level::kInfo);
    // TODO Log user/host/database
    LOG(log_level) << "Set '" << param << "' = '" << value << "' at "
                   << (is_transaction_scope ? "transaction" : "session")
                   << " scope";
    ExecuteCommand("select set_config($1, $2, $3)", param, value,
                   is_transaction_scope);
  }

  template <typename... T>
  ResultSet ExecuteCommand(const std::string& statement, const T&... args) {
    detail::QueryParameters params;
    params.Write(db_types_, args...);
    return ExecuteCommand(statement, params);
  }

  template <typename... T>
  ResultSet ExecuteCommand(CommandControl stmt_cmd_ctl,
                           const std::string& statement, const T&... args) {
    detail::QueryParameters params;
    params.Write(db_types_, args...);
    return ExecuteCommand(statement, params,
                          OptionalCommandControl{std::move(stmt_cmd_ctl)});
  }

  void CheckBusy() {
    if (GetConnectionState() == ConnectionState::kTranActive) {
      throw ConnectionBusy("There is another query in flight");
    }
  }

  // TODO Add tracing::Span
  ResultSet ExecuteCommand(const std::string& statement,
                           const detail::QueryParameters& params,
                           OptionalCommandControl statement_cmd_ctl = {}) {
    CheckBusy();
    TimeoutType network_timeout = statement_cmd_ctl.is_initialized()
                                      ? statement_cmd_ctl->network
                                      : CurrentNetworkTimeout();
    auto deadline = engine::Deadline::FromDuration(network_timeout);
    StatementTimeoutCentry centry{this, std::move(statement_cmd_ctl)};

    CountExecute count_execute(stats_);
    auto query_hash = QueryHash(statement, params);
    std::string statement_name = "q" + std::to_string(query_hash);

    if (prepared_.count(query_hash)) {
      LOG_TRACE() << "Query " << statement << " is already prepared.";
    } else {
      LOG_TRACE() << "Query " << statement << " is not yet prepared";
      conn_wrapper_.SendPrepare(statement_name, statement, params);
      try {
        conn_wrapper_.WaitResult(db_types_, deadline);
      } catch (DuplicatePreparedStatement const& e) {
        LOG_ERROR()
            << "Looks like your pg_bouncer doesn't clean up connections "
               "upon returning them to the pool. Please set pg_bouncer's "
               "pooling mode to `session` and `server_reset_query` parameter "
               "to `DISCARD ALL`. Please see documentation here "
               "https://nda.ya.ru/3UXMpu";
        throw;
      }
      conn_wrapper_.SendDescribePrepared(statement_name);
      auto res = conn_wrapper_.WaitResult(db_types_, deadline);
      prepared_.insert(std::make_pair(
          query_hash, res.GetRowDescription().BestReplyFormat(db_types_)));
      ++stats_.parse_total;
    }
    // TODO Get field descriptions from the prepare result and use them to
    // build text/binary format description
    auto fmt = prepared_[query_hash];
    LOG_TRACE() << "Use "
                << (fmt == io::DataFormat::kTextDataFormat ? "text" : "binary")
                << " format for reply";

    conn_wrapper_.SendPreparedQuery(statement_name, params, fmt);
    try {
      auto res = conn_wrapper_.WaitResult(db_types_, deadline);
      count_execute.AccountResult(res, fmt);
      return res;
    } catch (InvalidSqlStatementName const& e) {
      LOG_ERROR() << "Looks like your pg_bouncer is not in 'session' mode. "
                     "Please switch pg_bouncers's pooling mode to 'session'. "
                     "Please see documentation here https://nda.ya.ru/3UXMpu";
      throw;
    } catch (ConnectionTimeoutError const& e) {
      LOG_ERROR() << "Statement " << statement
                  << " network timeout error: " << e.what() << ". "
                  << "Network timout was " << network_timeout.count() << "ms";
      throw;
    }
  }

  // TODO Add tracing::Span
  ResultSet ExecuteCommandNoPrepare(const std::string& statement) {
    CheckBusy();
    CountExecute count_execute(stats_);
    conn_wrapper_.SendQuery(statement);

    auto res = conn_wrapper_.WaitResult(
        db_types_, engine::Deadline::FromDuration(CurrentNetworkTimeout()));
    count_execute.AccountResult(res, io::DataFormat::kTextDataFormat);
    return res;
  }

  /// A separate method from ExecuteCommand as the method will be transformed
  /// to parse-bind-execute pipeline. This method is for experimenting with
  /// PostreSQL protocol and is intentionally separate from usual path method.
  ResultSet ExperimentalExecute(const std::string& statement,
                                io::DataFormat reply_format,
                                const detail::QueryParameters& params) {
    conn_wrapper_.SendQuery(statement, params, reply_format);
    return conn_wrapper_.WaitResult(
        db_types_, engine::Deadline::FromDuration(CurrentNetworkTimeout()));
  }

  void Begin(const TransactionOptions& options,
             SteadyClock::time_point&& trx_start_time,
             OptionalCommandControl trx_cmd_ctl = {}) {
    if (IsInTransaction()) {
      throw AlreadyInTransaction();
    }
    stats_.trx_start_time = std::move(trx_start_time);
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

  void Cleanup(TimeoutType timeout) {
    {
      auto state = GetConnectionState();
      if (state == ConnectionState::kOffline) {
        return;
      }
      auto deadline = engine::Deadline::FromDuration(timeout);
      if (GetConnectionState() == ConnectionState::kTranActive) {
        auto cancel = conn_wrapper_.Cancel();
        // May throw on timeout
        try {
          conn_wrapper_.DiscardInput(deadline);
        } catch (std::exception const&) {
          // Consume error, we will throw later if we detect transaction is busy
        }
        cancel.WaitUntil(deadline);
      }
    }
    {
      auto state = GetConnectionState();
      if (state == ConnectionState::kTranActive) {
        throw ConnectionTimeoutError("Timeout while cleaning up connection");
      }
      if (state > ConnectionState::kIdle) {
        Rollback();
      }
      SetDefaultStatementTimeout(default_cmd_ctl_.statement);
    }
  }

  bool IsConnected() const {
    return GetConnectionState() > ConnectionState::kOffline;
  }

  bool IsIdle() const { return GetConnectionState() == ConnectionState::kIdle; }

  bool IsInTransaction() const {
    return GetConnectionState() > ConnectionState::kIdle;
  }

  const UserTypes& GetUserTypes() const { return db_types_; }
};  // Connection::Impl

std::unique_ptr<Connection> Connection::Connect(
    const std::string& conninfo, engine::TaskProcessor& bg_task_processor,
    uint32_t id, CommandControl default_cmd_ctl) {
  std::unique_ptr<Connection> conn(new Connection());

  conn->pimpl_ = std::make_unique<Impl>(bg_task_processor, id, default_cmd_ctl);
  conn->pimpl_->AsyncConnect(conninfo);

  return conn;
}

Connection::Connection() {}

Connection::~Connection() = default;

CommandControl Connection::GetDefaultCommandControl() const {
  return pimpl_->default_cmd_ctl_;
}

void Connection::SetDefaultCommandControl(CommandControl cmd_ctl) {
  pimpl_->SetDefaultCommandControl(cmd_ctl);
}

bool Connection::IsReadOnly() const { return pimpl_->read_only_; }

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

void Connection::SetParameter(const std::string& param,
                              const std::string& value, ParameterScope scope) {
  pimpl_->SetParameter(param, value, scope);
}

void Connection::ReloadUserTypes() { pimpl_->LoadUserTypes(); }

const UserTypes& Connection::GetUserTypes() const {
  return pimpl_->GetUserTypes();
}

const logging::LogExtra& Connection::GetLogExtra() const {
  return pimpl_->GetLogExtra();
}

ResultSet Connection::ExperimentalExecute(
    const std::string& statement, io::DataFormat reply_format,
    const detail::QueryParameters& params) {
  return pimpl_->ExperimentalExecute(statement, reply_format, params);
}

void Connection::Begin(const TransactionOptions& options,
                       SteadyClock::time_point&& trx_start_time,
                       OptionalCommandControl trx_cmd_ctl) {
  pimpl_->Begin(options, std::move(trx_start_time), std::move(trx_cmd_ctl));
}

void Connection::Commit() { pimpl_->Commit(); }

void Connection::Rollback() { pimpl_->Rollback(); }

void Connection::Cleanup(TimeoutType timeout) { pimpl_->Cleanup(timeout); }

}  // namespace detail
}  // namespace postgres
}  // namespace storages
