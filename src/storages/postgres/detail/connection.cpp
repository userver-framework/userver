#include <storages/postgres/detail/connection.hpp>

#include <engine/io/socket.hpp>
#include <logging/log.hpp>

#include <engine/async.hpp>
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
/// Size of error message buffer for PQcancel
/// 256 bytes is recommended in libpq documentation
/// https://www.postgresql.org/docs/10/static/libpq-cancel.html
const int kErrBufferSize = 256;

}  // namespace

// TODO Separate connection logic implementation and PGconn wrapper
struct Connection::Impl {
  using ResultHandle = detail::ResultSetImpl::ResultHandle;

  ConnectionState state_ = ConnectionState::kOffline;
  PGconn* conn_ = nullptr;
  engine::io::Socket socket_;
  engine::TaskProcessor& bg_task_processor_;

  Impl(engine::TaskProcessor& bg_task_processor)
      : bg_task_processor_(bg_task_processor) {}
  ~Impl() { Close().Detach(); }

  __attribute__((warn_unused_result)) engine::TaskWithResult<void> Close() {
    if (!conn_) {
      return engine::Async(bg_task_processor_, [] {});
    }

    engine::io::Socket tmp_sock{std::move(socket_)};
    socket_ = engine::io::Socket();
    state_ = ConnectionState::kOffline;

    PGconn* tmp_conn = std::exchange(conn_, nullptr);
    return engine::CriticalAsync(
        bg_task_processor_,
        [ tmp_conn, socket = std::move(tmp_sock) ]() mutable {
          PQfinish(tmp_conn);
          if (socket) {
            boost::ignore_unused(std::move(socket).Release());
          }
        });
  }

  void Cancel() {
    if (conn_ && state_ != ConnectionState::kOffline) {
      std::unique_ptr<PGcancel, decltype(&PQfreeCancel)> cancel{
          PQgetCancel(conn_), &PQfreeCancel};
      engine::Async(
          bg_task_processor_,
          [cancel = std::move(cancel)] {
            std::array<char, kErrBufferSize> buffer;
            if (!PQcancel(cancel.get(), buffer.data(), buffer.size())) {
              LOG_WARNING() << "Failed to cancel current request";
              // TODO Throw exception or not?
            }
          })
          .Get();
    }
  }

  // TODO Add tracing::Span
  void AsyncConnect(const std::string& conninfo) {
    conn_ = PQconnectStart(conninfo.c_str());
    if (!conn_) {
      // The only reason the pointer cannot be null is that libpq failed
      // to allocate memory for the structure
      // TODO Log error and throw appropriate exception
      throw std::bad_alloc{};
    }
    if (CONNECTION_BAD == GetConnectionStatus()) {
      Close().Get();
      // TODO Add connection info to the error
      LOG_DEBUG() << "Failed to start PostgreSQL connection";
      // TODO Add connection info to the error
      throw ConnectionFailed();
    }
    auto socket = PQsocket(conn_);
    if (socket < 0) {
      // TODO Add connection info to the error
      LOG_DEBUG() << "Invalid PostgreSQL connection socket";
      throw ConnectionFailed();
    }

    socket_ = engine::io::Socket(socket);
    WaitConnectionFinish();
  }

  void WaitConnectionFinish() {
    auto poll_res = PGRES_POLLING_WRITING;
    while (true) {
      switch (poll_res) {
        case PGRES_POLLING_READING:
          WaitSocketReadable(engine::Deadline::FromDuration(kDefaultTimeout));
          break;
        case PGRES_POLLING_WRITING:
          WaitSocketWriteable(engine::Deadline::FromDuration(kDefaultTimeout));
          break;
        case PGRES_POLLING_ACTIVE:
          LOG_WARNING()
              << "PGRES_POLLING_ACTIVE came, but it's obsolete. Ignoring...";
          break;
        case PGRES_POLLING_FAILED:
          throw ConnectionFailed();
          break;
        case PGRES_POLLING_OK:
          OnConnect();
          return;
      }
      poll_res = PQconnectPoll(conn_);
    }
  }

  void OnConnect() {
    if (PQsetnonblocking(conn_, 1)) {
      // TODO Deside on severity of the problem
      LOG_WARNING() << "Failed to set non-blocking connection mode";
    }

    LOG_DEBUG() << "PostgreSQL connected";
    state_ = GetConnectionState();
  }

  ConnStatusType GetConnectionStatus() const { return PQstatus(conn_); }

  PGTransactionStatusType GetTransactionStatus() const {
    return PQtransactionStatus(conn_);
  }

  ConnectionState GetConnectionState() const {
    switch (GetTransactionStatus()) {
      case PQTRANS_IDLE:
        return ConnectionState::kIdle;
      case PQTRANS_ACTIVE:
        return ConnectionState::kTranActive;
      case PQTRANS_INTRANS:
        return ConnectionState::kTranIdle;
      case PQTRANS_INERROR:
        return ConnectionState::kTranError;
      default:
        break;
    }
    return ConnectionState::kOffline;
  }

  void WaitSocketWriteable(engine::Deadline dl) { socket_.WaitWriteable(dl); }
  void WaitSocketReadable(engine::Deadline dl) { socket_.WaitReadable(dl); }

  void Flush() {
    while (const int flush_res = PQflush(conn_)) {
      if (flush_res < 0) {
        throw QueryError(PQerrorMessage(conn_));
      }
      WaitSocketWriteable(engine::Deadline::FromDuration(kDefaultTimeout));
    }
  }

  void ConsumeInput() {
    while (PQisBusy(conn_)) {
      WaitSocketReadable(engine::Deadline::FromDuration(kDefaultTimeout));
      CheckError<QueryError>(PQconsumeInput(conn_));
    }
  }

  template <typename ExceptionType>
  void CheckError(int pg_dispatch_result) {
    if (pg_dispatch_result == 0) {
      throw ExceptionType(PQerrorMessage(conn_));
    }
  }

  // TODO Add tracing::Span
  ResultSet ExecuteCommand(const std::string& statement,
                           const detail::QueryParameters& params) {
    if (params.Empty()) {
      CheckError<QueryError>(PQsendQueryParams(
          conn_, statement.c_str(), 0, nullptr, nullptr, nullptr, nullptr, 0));
    } else {
      CheckError<QueryError>(PQsendQueryParams(
          conn_, statement.c_str(), params.Size(), params.ParamTypesBuffer(),
          params.ParamBuffers(), params.ParamLengthsBuffer(),
          params.ParamFormatsBuffer(), 0));
    }
    return WaitResult();
  }

  // TODO Add tracing::Span
  ResultSet ExecuteCommand(const std::string& statement) {
    CheckError<QueryError>(PQsendQuery(conn_, statement.c_str()));
    return WaitResult();
  }

  /// A separate method from ExecuteCommand as the method will be transformed
  /// to parse-bind-execute pipeline. This method is for experimenting with
  /// PostreSQL protocol and is intentionally separate from usual path method.
  ResultSet ExperimentalExecute(const std::string& statement,
                                io::DataFormat reply_format,
                                const detail::QueryParameters& params) {
    if (params.Empty()) {
      CheckError<QueryError>(
          PQsendQueryParams(conn_, statement.c_str(), 0, nullptr, nullptr,
                            nullptr, nullptr, static_cast<int>(reply_format)));
    } else {
      CheckError<QueryError>(PQsendQueryParams(
          conn_, statement.c_str(), params.Size(), params.ParamTypesBuffer(),
          params.ParamBuffers(), params.ParamLengthsBuffer(),
          params.ParamFormatsBuffer(), static_cast<int>(reply_format)));
    }
    return WaitResult();
  }

  ResultSet WaitResult() {
    Flush();
    ResultHandle handle = MakeResultHandle(nullptr);
    ConsumeInput();
    auto pg_res = PQgetResult(conn_);
    while (pg_res != nullptr) {
      // All but the last result sets are discarded
      if (handle)
        LOG_DEBUG()
            << "Query returned several result sets, a result set is discarded";
      handle = MakeResultHandle(pg_res);

      ConsumeInput();
      pg_res = PQgetResult(conn_);
      // TODO check notifications
    }
    return MakeResult(std::move(handle));
  }

  ResultHandle MakeResultHandle(PGresult* pg_res) const {
    return {pg_res, &PQclear};
  }

  ResultSet MakeResult(ResultHandle&& handle) {
    auto status = PQresultStatus(handle.get());
    switch (status) {
      case PGRES_EMPTY_QUERY:
        throw QueryError{"Empty query"};
      case PGRES_COMMAND_OK:
        LOG_DEBUG() << "Successful completion of a command returning no data";
        break;
      case PGRES_COPY_IN:
      case PGRES_COPY_OUT:
      case PGRES_COPY_BOTH:
        Close().Get();
        // TODO some logic error
        throw QueryError{"Copy is not implemented"};
      case PGRES_BAD_RESPONSE:
        Close().Get();
        throw ConnectionError{"Failed to parse server response"};
      case PGRES_NONFATAL_ERROR:
        LOG_DEBUG() << "Non-fatal error occurred";
        break;
      case PGRES_FATAL_ERROR: {
        // TODO Retrieve error information and throw an appropriate exception
        auto msg = PQresultErrorMessage(handle.get());
        LOG_ERROR() << "Fatal error occured " << msg;
        throw QueryError{msg};
      }
      default:
        break;
    }
    return ResultSet{
        std::make_shared<detail::ResultSetImpl>(std::move(handle))};
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
