#include <storages/postgres/detail/pg_connection_wrapper.hpp>

#include <storages/postgres/exceptions.hpp>
#include <storages/postgres/message.hpp>

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
void PGConnectionWrapper::CheckError(const std::string& cmd,
                                     int pg_dispatch_result) {
  if (pg_dispatch_result == 0) {
    auto msg = PQerrorMessage(conn_);
    LOG_ERROR() << "libpq " << cmd << " error: " << msg;
    throw ExceptionType(cmd + " execution error: " + msg);
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
          [[maybe_unused]] int fd = std::move(socket).Release();
        }
      });
}

template <typename ExceptionType>
void PGConnectionWrapper::CloseWithError(ExceptionType&& ex) {
  Close().Get();
  throw std::move(ex);
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
    throw ConnectionFailed{conninfo, "Failed to allocate PGconn structure"};
  }
  if (CONNECTION_BAD == PQstatus(conn_)) {
    LOG_DEBUG() << "Failed to start a PostgreSQL connection to " << conninfo;
    CloseWithError(
        ConnectionFailed{conninfo, "Failed to start libpq connection"});
  }
  const auto socket = PQsocket(conn_);
  if (socket < 0) {
    LOG_ERROR() << "Invalid PostgreSQL socket " << socket
                << " when connecting to " << conninfo;
    throw ConnectionFailed{conninfo, "Invalid socket handle"};
  }
  socket_ = engine::io::Socket(socket);
}

void PGConnectionWrapper::WaitConnectionFinish(const std::string& conninfo,
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
        LOG_ERROR() << conninfo << " libpq polling failed";
        throw ConnectionFailed{conninfo, "libpq connection polling failed"};
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
      throw CommandError(PQerrorMessage(conn_));
    }
    WaitSocketWriteable(timeout);
  }
}

void PGConnectionWrapper::ConsumeInput(Duration timeout) {
  while (PQisBusy(conn_)) {
    WaitSocketReadable(timeout);
    CheckError<CommandError>("PQconsumeInput", PQconsumeInput(conn_));
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
  auto wrapper = std::make_shared<detail::ResultWrapper>(std::move(handle));
  auto status = wrapper->GetStatus();
  switch (status) {
    case PGRES_EMPTY_QUERY:
      throw LogicError{"Empty query"};
    case PGRES_COMMAND_OK:
      LOG_TRACE() << "Successful completion of a command returning no data";
      break;
    case PGRES_TUPLES_OK:
      LOG_TRACE() << "Successful completion of a command returning data";
      break;
    case PGRES_SINGLE_TUPLE:
      LOG_ERROR()
          << "libpq was switched to SINGLE_ROW mode, this is not supported.";
      CloseWithError(NotImplemented{"Single row mode is not supported"});
    case PGRES_COPY_IN:
    case PGRES_COPY_OUT:
    case PGRES_COPY_BOTH:
      LOG_ERROR() << "PostgreSQL COPY command invoked which is not implemented"
                  << logging::LogExtra::Stacktrace();
      CloseWithError(NotImplemented{"Copy is not implemented"});
    case PGRES_BAD_RESPONSE:
      CloseWithError(ConnectionError{"Failed to parse server response"});
    case PGRES_NONFATAL_ERROR: {
      Message msg{wrapper};
      switch (msg.GetSeverity()) {
        case Message::Severity::kDebug:
          LOG_DEBUG() << "Postgres " << msg.GetSeverityString()
                      << " message: " << msg.GetMessage() << msg.GetLogExtra();
          break;
        case Message::Severity::kLog:
        case Message::Severity::kInfo:
        case Message::Severity::kNotice:
          LOG_INFO() << "Postgres " << msg.GetSeverityString()
                     << " message: " << msg.GetMessage() << msg.GetLogExtra();
          break;
        case Message::Severity::kWarning:
          LOG_WARNING() << "Postgres " << msg.GetSeverityString()
                        << " message: " << msg.GetMessage()
                        << msg.GetLogExtra();
          break;
        case Message::Severity::kError:
        case Message::Severity::kFatal:
        case Message::Severity::kPanic:
          LOG_ERROR() << "Postgres " << msg.GetSeverityString()
                      << " message (marked as non-fatal): " << msg.GetMessage()
                      << msg.GetLogExtra();
          break;
      }
      break;
    }
    case PGRES_FATAL_ERROR: {
      Message msg{wrapper};
      if (!IsWhitelistedState(msg.GetSqlState())) {
        LOG_ERROR() << "Fatal error occured: " << msg.GetMessage()
                    << msg.GetLogExtra();
      } else {
        LOG_WARNING() << "Fatal error occured: " << msg.GetMessage()
                      << msg.GetLogExtra();
      }
      msg.ThrowException();
      break;
    }
  }
  return ResultSet{wrapper};
}

void PGConnectionWrapper::SendQuery(const std::string& statement) {
  CheckError<CommandError>("PQsendQuery",
                           PQsendQuery(conn_, statement.c_str()));
}

void PGConnectionWrapper::SendQuery(const std::string& statement,
                                    const QueryParameters& params,
                                    io::DataFormat reply_format) {
  if (params.Empty()) {
    CheckError<CommandError>(
        "PQsendQueryParams",
        PQsendQueryParams(conn_, statement.c_str(), 0, nullptr, nullptr,
                          nullptr, nullptr, static_cast<int>(reply_format)));
  } else {
    CheckError<CommandError>(
        "PQsendQueryParams",
        PQsendQueryParams(
            conn_, statement.c_str(), params.Size(), params.ParamTypesBuffer(),
            params.ParamBuffers(), params.ParamLengthsBuffer(),
            params.ParamFormatsBuffer(), static_cast<int>(reply_format)));
  }
}

void PGConnectionWrapper::SendPrepare(const std::string& name,
                                      const std::string& statement,
                                      const QueryParameters& params) {
  if (params.Empty()) {
    CheckError<CommandError>(
        "PQsendPrepare",
        PQsendPrepare(conn_, name.c_str(), statement.c_str(), 0, nullptr));
  } else {
    CheckError<CommandError>(
        "PQsendPrepare",
        PQsendPrepare(conn_, name.c_str(), statement.c_str(), params.Size(),
                      params.ParamTypesBuffer()));
  }
}

void PGConnectionWrapper::SendDescribePrepared(const std::string& name) {
  CheckError<CommandError>("PQsendDescribePrepared",
                           PQsendDescribePrepared(conn_, name.c_str()));
}

void PGConnectionWrapper::SendPreparedQuery(const std::string& name,
                                            const QueryParameters& params,
                                            io::DataFormat reply_format) {
  if (params.Empty()) {
    CheckError<CommandError>(
        "PQsendQueryPrepared",
        PQsendQueryPrepared(conn_, name.c_str(), 0, nullptr, nullptr, nullptr,
                            static_cast<int>(reply_format)));
  } else {
    CheckError<CommandError>(
        "PQsendQueryPrepared",
        PQsendQueryPrepared(conn_, name.c_str(), params.Size(),
                            params.ParamBuffers(), params.ParamLengthsBuffer(),
                            params.ParamFormatsBuffer(),
                            static_cast<int>(reply_format)));
  }
}

}  // namespace detail
}  // namespace postgres
}  // namespace storages
