#include <storages/postgres/detail/pg_connection_wrapper.hpp>

#include <boost/core/ignore_unused.hpp>

#include <storages/postgres/exceptions.hpp>

#include <logging/log.hpp>
#include <logging/log_extra.hpp>

namespace storages {
namespace postgres {
namespace detail {

namespace {
/// Size of error message buffer for PQcancel
/// 256 bytes is recommended in libpq documentation
/// https://www.postgresql.org/docs/10/static/libpq-cancel.html
const int kErrBufferSize = 256;
}  // namespace

template <typename ExceptionType>
void PGConnectionWrapper::CheckError(int pg_dispatch_result) {
  if (pg_dispatch_result == 0) {
    throw ExceptionType(PQerrorMessage(conn_));
  }
}

PGTransactionStatusType PGConnectionWrapper::GetTransactionStatus() const {
  return PQtransactionStatus(conn_);
}

ConnectionState PGConnectionWrapper::GetConnectionState() const {
  if (!conn_) {
    return ConnectionState::kOffline;
  }
  switch (GetTransactionStatus()) {
    case PQTRANS_IDLE:
      return ConnectionState::kIdle;
    case PQTRANS_ACTIVE:
      return ConnectionState::kTranActive;
    case PQTRANS_INTRANS:
      return ConnectionState::kTranIdle;
    case PQTRANS_INERROR:
      return ConnectionState::kTranError;
    case PQTRANS_UNKNOWN:
      return ConnectionState::kOffline;
  }
}

engine::TaskWithResult<void> PGConnectionWrapper::Close() {
  if (!conn_) {
    // TODO avoid empty task
    return engine::Async(bg_task_processor_, [] {});
  }

  engine::io::Socket tmp_sock{std::move(socket_)};
  socket_ = engine::io::Socket();

  PGconn* tmp_conn = std::exchange(conn_, nullptr);
  return engine::CriticalAsync(
      bg_task_processor_, [ tmp_conn, socket = std::move(tmp_sock) ]() mutable {
        PQfinish(tmp_conn);
        if (socket) {
          boost::ignore_unused(std::move(socket).Release());
        }
      });
}

engine::TaskWithResult<void> PGConnectionWrapper::Cancel() {
  if (ConnectionState::kOffline == GetConnectionState()) {
    // TODO avoid empty task
    return engine::Async(bg_task_processor_, [] {});
  }
  std::unique_ptr<PGcancel, decltype(&PQfreeCancel)> cancel{PQgetCancel(conn_),
                                                            &PQfreeCancel};
  return engine::Async(bg_task_processor_, [cancel = std::move(cancel)] {
    std::array<char, kErrBufferSize> buffer;
    if (!PQcancel(cancel.get(), buffer.data(), buffer.size())) {
      LOG_WARNING() << "Failed to cancel current request";
      // TODO Throw exception or not?
    }
  });
}

void PGConnectionWrapper::AsyncConnect(const std::string& conninfo,
                                       Duration poll_timeout) {
  StartAsyncConnect(conninfo);
  WaitConnectionFinish(conninfo, poll_timeout);
  OnConnect();
  LOG_DEBUG() << "Connected to " << conninfo;
}

void PGConnectionWrapper::StartAsyncConnect(const std::string& conninfo) {
  if (conn_) {
    LOG_ERROR() << "Attempt to connect a connection that is already connected"
                << logging::LogExtra::Stacktrace();
    throw ConnectionError{"Already connected"};
  }
  conn_ = PQconnectStart(conninfo.c_str());
  if (!conn_) {
    // The only reason the pointer cannot be null is that libpq failed
    // to allocate memory for the structure
    LOG_ERROR() << "libpq failed to allocate a PGconn structure"
                << logging::LogExtra::Stacktrace();
    // TODO throw appropriate exception
    throw std::bad_alloc{};
  }
  if (CONNECTION_BAD == PQstatus(conn_)) {
    Close().Get();
    LOG_DEBUG() << "Failed to start a PostgreSQL connection to " << conninfo;
    // TODO Add connection info to the exception
    throw ConnectionFailed{};
  }
  const auto socket = PQsocket(conn_);
  if (socket < 0) {
    LOG_ERROR() << "Invalid PostgreSQL socket " << socket
                << " when connecting to " << conninfo;
    // TODO Add connection info to the exception
    throw ConnectionFailed{};
  }
  socket_ = engine::io::Socket(socket);
}

void PGConnectionWrapper::WaitConnectionFinish(const std::string& /*conninfo*/,
                                               Duration poll_timeout) {
  auto poll_res = PGRES_POLLING_WRITING;
  while (poll_res != PGRES_POLLING_OK) {
    switch (poll_res) {
      case PGRES_POLLING_READING:
        WaitSocketReadable(poll_timeout);
        break;
      case PGRES_POLLING_WRITING:
        WaitSocketWriteable(poll_timeout);
        break;
      case PGRES_POLLING_ACTIVE:
        // This is an obsolete state, just ignore it
        break;
      case PGRES_POLLING_FAILED:
        // TODO Add connection info to the exception. Log failure
        throw ConnectionFailed{};
      default:
        assert(!"Unexpected enumeration value");
        break;
    }
    poll_res = PQconnectPoll(conn_);
  }
}

void PGConnectionWrapper::OnConnect() {
  if (PQsetnonblocking(conn_, 1)) {
    // TODO Deside on severity of the problem
    LOG_WARNING() << "Failed to set non-blocking connection mode";
  }
}

void PGConnectionWrapper::WaitSocketReadable(Duration timeout) {
  socket_.WaitReadable(engine::Deadline::FromDuration(timeout));
}

void PGConnectionWrapper::WaitSocketWriteable(Duration timeout) {
  socket_.WaitWriteable(engine::Deadline::FromDuration(timeout));
}

void PGConnectionWrapper::Flush(Duration timeout) {
  while (const int flush_res = PQflush(conn_)) {
    if (flush_res < 0) {
      throw QueryError(PQerrorMessage(conn_));
    }
    WaitSocketWriteable(timeout);
  }
}

void PGConnectionWrapper::ConsumeInput(Duration timeout) {
  while (PQisBusy(conn_)) {
    WaitSocketReadable(timeout);
    CheckError<QueryError>(PQconsumeInput(conn_));
  }
}

ResultSet PGConnectionWrapper::WaitResult(Duration timeout) {
  Flush(timeout);
  auto handle = MakeResultHandle(nullptr);
  ConsumeInput(timeout);
  while (auto pg_res = PQgetResult(conn_)) {
    if (handle) {
      // TODO Decide about the severity of this situation
      LOG_DEBUG()
          << "Query returned several result sets, a result set is discarded";
    }
    handle = MakeResultHandle(pg_res);
    ConsumeInput(timeout);
  }
  return MakeResult(std::move(handle));
}

ResultSet PGConnectionWrapper::MakeResult(ResultHandle&& handle) {
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
      assert(!"Unexpected enumeration value");
      break;
  }
  return ResultSet{std::make_shared<detail::ResultSetImpl>(std::move(handle))};
}

void PGConnectionWrapper::SendQuery(const std::string& statement) {
  // TODO QuerySendError
  CheckError<QueryError>(PQsendQuery(conn_, statement.c_str()));
}

void PGConnectionWrapper::SendQuery(const std::string& statement,
                                    const QueryParameters& params,
                                    io::DataFormat reply_format) {
  if (params.Empty()) {
    // TODO QuerySendError
    CheckError<QueryError>(PQsendQueryParams(conn_, statement.c_str(), 0,
                                             nullptr, nullptr, nullptr, nullptr,
                                             static_cast<int>(reply_format)));
  } else {
    // TODO QuerySendError
    CheckError<QueryError>(PQsendQueryParams(
        conn_, statement.c_str(), params.Size(), params.ParamTypesBuffer(),
        params.ParamBuffers(), params.ParamLengthsBuffer(),
        params.ParamFormatsBuffer(), static_cast<int>(reply_format)));
  }
}

}  // namespace detail
}  // namespace postgres
}  // namespace storages
