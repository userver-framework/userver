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

}  // namespace

struct Connection::Impl {
  using PreparedStatements = std::unordered_map<std::size_t, io::DataFormat>;

  PGConnectionWrapper conn_wrapper_;
  PreparedStatements prepared_;
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

  // TODO Add tracing::Span
  void AsyncConnect(const std::string& conninfo) {
    conn_wrapper_.AsyncConnect(conninfo, kDefaultTimeout);
    // We cannot handle exceptions here, so we let them got to the caller
    // Turn off error messages localisation
    SetParameter("lc_messages", "C");
    // Detect if the connection is read only.
    auto res = ExecuteCommand("show transaction_read_only");
    if (res) {
      res.Front().To(read_only_);
    }
    SetLocalTimezone();
  }

  /// @brief Set local timezone. If the timezone is invalid, catch the exception
  /// and log error.
  void SetLocalTimezone() {
    const auto& tz = LocalTimezoneID();
    LOG_TRACE() << "Try and set pg timezone id " << tz.id << " canonical id "
                << tz.canonical_id;
    const auto& tz_id = tz.canonical_id.empty() ? tz.id : tz.canonical_id;
    try {
      SetParameter("TimeZone", tz_id);
    } catch (const DataException&) {
      // No need to log the DataException message, it has been already logged
      // by connection wrapper.
      LOG_ERROR() << "Invalid value for time zone " << tz_id;
    }  // Let all other exceptions be propagated to the caller
  }

  ConnectionState GetConnectionState() const {
    return conn_wrapper_.GetConnectionState();
  }

  void SetParameter(const std::string& param, const std::string& value) {
    LOG_DEBUG() << "Set '" << param << "' = '" << value << "'";
    ExecuteCommand("update pg_settings set setting = $1 where name = $2", value,
                   param);
  }

  template <typename... T>
  ResultSet ExecuteCommand(const std::string& statement, const T&... args) {
    detail::QueryParameters params;
    params.Write(args...);
    return ExecuteCommand(statement, params);
  }

  // TODO Add tracing::Span
  ResultSet ExecuteCommand(const std::string& statement,
                           const detail::QueryParameters& params) {
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
          query_hash, res.GetRowDescription().BestReplyFormat()));
    }
    // TODO Get field descriptions from the prepare result and use them to
    // build text/binary format description
    auto fmt = prepared_[query_hash];
    LOG_TRACE() << "Use "
                << (fmt == io::DataFormat::kTextDataFormat ? "text" : "binary")
                << " format for reply";
    conn_wrapper_.SendPreparedQuery(statement_name, params, fmt);
    return conn_wrapper_.WaitResult(kDefaultTimeout);
  }

  // TODO Add tracing::Span
  ResultSet ExecuteCommand(const std::string& statement) {
    conn_wrapper_.SendQuery(statement);
    return conn_wrapper_.WaitResult(kDefaultTimeout);
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

  void Begin(const TransactionOptions& options) {
    if (GetConnectionState() > ConnectionState::kIdle)
      throw AlreadyInTransaction();
    ExecuteCommand(BeginStatement(options));
  }

  void Commit() {
    if (GetConnectionState() < ConnectionState::kTranIdle)
      throw NotInTransaction();
    ExecuteCommand("commit");
  }

  void Rollback() {
    if (GetConnectionState() < ConnectionState::kTranIdle)
      throw NotInTransaction();
    ExecuteCommand("rollback");
  }
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

bool Connection::IsConnected() const {
  return pimpl_->GetConnectionState() > ConnectionState::kOffline;
}
bool Connection::IsIdle() const {
  return pimpl_->GetConnectionState() == ConnectionState::kIdle;
}

bool Connection::IsInTransaction() const {
  return pimpl_->GetConnectionState() > ConnectionState::kIdle;
}

void Connection::Close() { pimpl_->Close().Get(); }

ResultSet Connection::Execute(const std::string& statement,
                              const detail::QueryParameters& params) {
  return pimpl_->ExecuteCommand(statement, params);
}

void Connection::SetParameter(const std::string& param,
                              const std::string& value) {
  pimpl_->SetParameter(param, value);
}

ResultSet Connection::ExperimentalExecute(
    const std::string& statement, io::DataFormat reply_format,
    const detail::QueryParameters& params) {
  return pimpl_->ExperimentalExecute(statement, reply_format, params);
}

void Connection::Begin(const TransactionOptions& options) {
  pimpl_->Begin(options);
}

void Connection::Commit() { pimpl_->Commit(); }

void Connection::Rollback() { pimpl_->Rollback(); }

}  // namespace detail
}  // namespace postgres
}  // namespace storages
