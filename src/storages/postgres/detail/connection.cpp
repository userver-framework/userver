#include <storages/postgres/detail/connection.hpp>

#include <unordered_map>

#include <boost/core/ignore_unused.hpp>
#include <boost/functional/hash.hpp>

#include <postgresql/libpq-fe.h>

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

// TODO Move the timeout constant to config
const std::chrono::seconds kDefaultTimeout(2);

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
        t.typnotnull,
        t.typtypmod,
        t.typndims
from pg_catalog.pg_type t
  left join pg_catalog.pg_namespace n on n.oid = t.typnamespace
where n.nspname not in ('pg_catalog', 'pg_toast', 'information_schema'))~";

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

  Impl(engine::TaskProcessor& bg_task_processor)
      : conn_wrapper_{bg_task_processor} {}
  ~Impl() {
    if (ConnectionState::kOffline != GetConnectionState()) {
      Close().Detach();
    }
  }

  __attribute__((warn_unused_result)) engine::TaskWithResult<void> Close() {
    return conn_wrapper_.Close();
  }

  void Cancel() { conn_wrapper_.Cancel().Get(); }

  Connection::Statistics GetStatsAndReset() {
    assert(!IsInTransaction() &&
           "GetStatsAndReset should be called outside of transaction");
    return std::exchange(stats_, Connection::Statistics{});
  }

  // TODO Add tracing::Span
  void AsyncConnect(const std::string& conninfo) {
    conn_wrapper_.AsyncConnect(conninfo, kDefaultTimeout);
    // We cannot handle exceptions here, so we let them got to the caller
    // Detect if the connection is read only.
    auto res = ExecuteCommandNoPrepare("show transaction_read_only");
    if (res) {
      res.Front().To(read_only_);
    }
    SetLocalTimezone();
    LoadUserTypes();
  }

  /// @brief Set local timezone. If the timezone is invalid, catch the exception
  /// and log error.
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

  void LoadUserTypes() {
    try {
      auto res = ExecuteCommand(kGetUserTypesSQL).As<DBTypeDescription>();
      db_types_.Reset();
      for (auto desc : res) {
        db_types_.AddType(std::move(desc));
      }
    } catch (const Error& e) {
      LOG_ERROR() << "Error loading user datatypes: " << e.what();
      // TODO Decide about rethrowing
      throw;
    }
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

  // TODO Add tracing::Span
  ResultSet ExecuteCommand(const std::string& statement,
                           const detail::QueryParameters& params) {
    CountExecute count_execute(stats_);
    auto query_hash = QueryHash(statement, params);
    std::string statement_name = "q" + std::to_string(query_hash);

    if (prepared_.count(query_hash)) {
      LOG_TRACE() << "Query " << statement << " is already prepared.";
    } else {
      LOG_TRACE() << "Query " << statement << " is not yet prepared";
      conn_wrapper_.SendPrepare(statement_name, statement, params);
      conn_wrapper_.WaitResult(kDefaultTimeout);
      conn_wrapper_.SendDescribePrepared(statement_name);
      auto res = conn_wrapper_.WaitResult(kDefaultTimeout);
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
    auto res = conn_wrapper_.WaitResult(kDefaultTimeout);
    count_execute.AccountResult(res, fmt);
    return res;
  }

  // TODO Add tracing::Span
  ResultSet ExecuteCommandNoPrepare(const std::string& statement) {
    CountExecute count_execute(stats_);
    conn_wrapper_.SendQuery(statement);

    auto res = conn_wrapper_.WaitResult(kDefaultTimeout);
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
    return conn_wrapper_.WaitResult(kDefaultTimeout);
  }

  void Begin(const TransactionOptions& options,
             SteadyClock::time_point&& trx_start_time) {
    if (IsInTransaction()) {
      throw AlreadyInTransaction();
    }
    stats_.trx_start_time = std::move(trx_start_time);
    ++stats_.trx_total;
    ExecuteCommandNoPrepare(BeginStatement(options));
  }

  void Commit() {
    if (!IsInTransaction()) {
      throw NotInTransaction();
    }
    CountCommit count_commit(stats_);
    ExecuteCommandNoPrepare("commit");
  }

  void Rollback() {
    if (!IsInTransaction()) {
      throw NotInTransaction();
    }
    CountRollback count_rollback(stats_);
    ExecuteCommandNoPrepare("rollback");
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
    const std::string& conninfo, engine::TaskProcessor& bg_task_processor) {
  std::unique_ptr<Connection> conn(new Connection());

  conn->pimpl_ = std::make_unique<Impl>(bg_task_processor);
  conn->pimpl_->AsyncConnect(conninfo);

  return conn;
}

Connection::Connection() {}

Connection::~Connection() = default;

bool Connection::IsReadOnly() const { return pimpl_->read_only_; }

ConnectionState Connection::GetState() const {
  return pimpl_->GetConnectionState();
}

bool Connection::IsConnected() const { return pimpl_->IsConnected(); }

bool Connection::IsIdle() const { return pimpl_->IsIdle(); }

bool Connection::IsInTransaction() const { return pimpl_->IsInTransaction(); }

void Connection::Close() { pimpl_->Close().Get(); }

Connection::Statistics Connection::GetStatsAndReset() {
  return pimpl_->GetStatsAndReset();
}

ResultSet Connection::Execute(const std::string& statement,
                              const detail::QueryParameters& params) {
  return pimpl_->ExecuteCommand(statement, params);
}

void Connection::SetParameter(const std::string& param,
                              const std::string& value, ParameterScope scope) {
  pimpl_->SetParameter(param, value, scope);
}

void Connection::ReloadUserTypes() { pimpl_->LoadUserTypes(); }
const UserTypes& Connection::GetUserTypes() const {
  return pimpl_->GetUserTypes();
}

ResultSet Connection::ExperimentalExecute(
    const std::string& statement, io::DataFormat reply_format,
    const detail::QueryParameters& params) {
  return pimpl_->ExperimentalExecute(statement, reply_format, params);
}

void Connection::Begin(const TransactionOptions& options,
                       SteadyClock::time_point&& trx_start_time) {
  pimpl_->Begin(options, std::move(trx_start_time));
}

void Connection::Commit() { pimpl_->Commit(); }

void Connection::Rollback() { pimpl_->Rollback(); }

}  // namespace detail
}  // namespace postgres
}  // namespace storages
