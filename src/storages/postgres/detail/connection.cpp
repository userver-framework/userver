#include <storages/postgres/detail/connection.hpp>

#include <engine/io/socket.hpp>
#include <logging/log.hpp>

#include <engine/async.hpp>
#include <storages/postgres/detail/pg_connection_wrapper.hpp>
#include <storages/postgres/detail/result_set_impl.hpp>
#include <storages/postgres/exceptions.hpp>

#include <postgresql/libpq-fe.h>

#include <boost/core/ignore_unused.hpp>

namespace storages {
namespace postgres {
namespace detail {

namespace {

// TODO Move the timeout constant to config
const std::chrono::seconds kDefaultTimeout(2);

}  // namespace

struct Connection::Impl {
  PGConnectionWrapper conn_wrapper_;

  Impl(engine::TaskProcessor& bg_task_processor)
      : conn_wrapper_{bg_task_processor} {}
  ~Impl() { Close().Detach(); }

  __attribute__((warn_unused_result)) engine::TaskWithResult<void> Close() {
    return conn_wrapper_.Close();
  }

  void Cancel() { conn_wrapper_.Cancel().Get(); }

  // TODO Add tracing::Span
  void AsyncConnect(const std::string& conninfo) {
    conn_wrapper_.AsyncConnect(conninfo, kDefaultTimeout);
  }

  ConnectionState GetConnectionState() const {
    return conn_wrapper_.GetConnectionState();
  }

  // TODO Add tracing::Span
  ResultSet ExecuteCommand(const std::string& statement,
                           const detail::QueryParameters& params) {
    conn_wrapper_.SendQuery(statement, params);
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
