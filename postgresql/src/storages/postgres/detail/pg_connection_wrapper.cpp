#include <storages/postgres/detail/pg_connection_wrapper.hpp>

#include <storages/postgres/dsn.hpp>
#include <storages/postgres/exceptions.hpp>
#include <storages/postgres/message.hpp>

#include <logging/log.hpp>

namespace storages {
namespace postgres {
namespace detail {

namespace {
/// Size of error message buffer for PQcancel
/// 256 bytes is recommended in libpq documentation
/// https://www.postgresql.org/docs/10/static/libpq-cancel.html
constexpr int kErrBufferSize = 256;
// TODO move to config
constexpr bool kVerboseErrors = false;

const char* MsgForStatus(ConnStatusType status) {
  switch (status) {
    case CONNECTION_OK:
      return "PQstatus: Connection established";
    case CONNECTION_BAD:
      return "PQstatus: Failed to start a connection";
    case CONNECTION_STARTED:
      return "PQstatus: Waiting for connection to be made";
    case CONNECTION_MADE:
      return "PQstatus: Connection OK; waiting to send";
    case CONNECTION_AWAITING_RESPONSE:
      return "PQstatus: Waiting for a response from the server";
    case CONNECTION_AUTH_OK:
      return "PQstatus: Received authentication; waiting for backend start-up";
    case CONNECTION_SETENV:
      return "PQstatus: Negotiating environment settings";
    case CONNECTION_SSL_STARTUP:
      return "PQstatus: Negotiating SSL";
    case CONNECTION_NEEDED:
      return "PQstatus: Internal state: connect() needed";
    case CONNECTION_CHECK_WRITABLE:
      return "PQstatus: Checking connection to handle writes";
    case CONNECTION_CONSUME:
      return "PQstatus: Consuming remaining response messages on connection";
  }
}

}  // namespace

PGConnectionWrapper::PGConnectionWrapper(engine::TaskProcessor& tp, uint32_t id)
    : bg_task_processor_{tp} {
  // TODO add SSL initialization
  log_extra_.Extend("conn_id", id);
}

template <typename ExceptionType>
void PGConnectionWrapper::CheckError(const std::string& cmd,
                                     int pg_dispatch_result) {
  if (pg_dispatch_result == 0) {
    auto msg = PQerrorMessage(conn_);
    LOG_ERROR() << log_extra_ << "libpq " << cmd << " error: " << msg;
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
  LOG_DEBUG() << log_extra_
              << "Closing connection because of failure: " << ex.what();
  Close().Get();
  throw std::forward<ExceptionType>(ex);
}

engine::TaskWithResult<void> PGConnectionWrapper::Cancel() {
  if (ConnectionState::kOffline == GetConnectionState()) {
    // TODO avoid empty task
    return engine::Async(bg_task_processor_, [] {});
  }
  std::unique_ptr<PGcancel, decltype(&PQfreeCancel)> cancel{PQgetCancel(conn_),
                                                            &PQfreeCancel};
  return engine::Async(
      bg_task_processor_, [ this, cancel = std::move(cancel) ] {
        std::array<char, kErrBufferSize> buffer;
        if (!PQcancel(cancel.get(), buffer.data(), buffer.size())) {
          LOG_WARNING() << log_extra_ << "Failed to cancel current request";
          // TODO Throw exception or not?
        }
      });
}

void PGConnectionWrapper::AsyncConnect(const std::string& conninfo,
                                       Duration poll_timeout) {
  LOG_DEBUG() << log_extra_ << "Connecting to " << DsnCutPassword(conninfo);
  StartAsyncConnect(conninfo);
  WaitConnectionFinish(poll_timeout);
  LOG_DEBUG() << log_extra_ << "Connected to " << DsnCutPassword(conninfo);
}

void PGConnectionWrapper::StartAsyncConnect(const std::string& conninfo) {
  if (conn_) {
    LOG_ERROR() << log_extra_
                << "Attempt to connect a connection that is already connected"
                << logging::LogExtra::Stacktrace();
    throw ConnectionFailed{conninfo, "Already connected"};
  }

  conn_ = PQconnectStart(conninfo.c_str());
  if (!conn_) {
    // The only reason the pointer cannot be null is that libpq failed
    // to allocate memory for the structure
    LOG_ERROR() << log_extra_ << "libpq failed to allocate a PGconn structure"
                << logging::LogExtra::Stacktrace();
    throw ConnectionFailed{conninfo, "Failed to allocate PGconn structure"};
  }

  if (PQsetnonblocking(conn_, 1)) {
    LOG_ERROR() << log_extra_
                << "libpq failed to set non-blocking connection mode";
    throw ConnectionFailed{conninfo,
                           "Failed to set non-blocking connection mode"};
  }

  const auto status = PQstatus(conn_);
  const auto* msg_for_status = MsgForStatus(status);
  if (CONNECTION_BAD == status) {
    const std::string msg = msg_for_status;
    LOG_ERROR() << log_extra_ << msg;
    CloseWithError(ConnectionFailed{conninfo, msg});
  } else {
    LOG_TRACE() << log_extra_ << msg_for_status;
  }

  const auto socket = PQsocket(conn_);
  if (socket < 0) {
    LOG_ERROR() << log_extra_ << "Invalid PostgreSQL socket " << socket
                << " while connecting";
    throw ConnectionFailed{conninfo, "Invalid socket handle"};
  }
  socket_ = engine::io::Socket(socket);

  const auto options = OptionsFromDsn(conninfo);
  log_extra_.Extend("host", options.host + ':' + options.port);
  log_extra_.Extend("dbname", options.dbname);

  if (kVerboseErrors) {
    PQsetErrorVerbosity(conn_, PQERRORS_VERBOSE);
  }
}

void PGConnectionWrapper::WaitConnectionFinish(Duration poll_timeout) {
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
        LOG_ERROR() << log_extra_ << " libpq polling failed";
        CheckError<ConnectionError>("PQconnectPoll", 0);
      default:
        assert(!"Unexpected enumeration value");
        break;
    }
    poll_res = PQconnectPoll(conn_);
    LOG_TRACE() << log_extra_ << MsgForStatus(PQstatus(conn_));
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

ResultSet PGConnectionWrapper::WaitResult(const UserTypes& types,
                                          Duration timeout) {
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
  return MakeResult(types, std::move(handle));
}

const logging::LogExtra& PGConnectionWrapper::GetLogExtra() const {
  return log_extra_;
}

ResultSet PGConnectionWrapper::MakeResult(const UserTypes& types,
                                          ResultHandle&& handle) {
  auto wrapper = std::make_shared<detail::ResultWrapper>(std::move(handle));
  auto status = wrapper->GetStatus();
  switch (status) {
    case PGRES_EMPTY_QUERY:
      throw LogicError{"Empty query"};
    case PGRES_COMMAND_OK:
      LOG_TRACE() << log_extra_
                  << "Successful completion of a command returning no data";
      break;
    case PGRES_TUPLES_OK:
      LOG_TRACE() << log_extra_
                  << "Successful completion of a command returning data";
      break;
    case PGRES_SINGLE_TUPLE:
      LOG_ERROR()
          << "libpq was switched to SINGLE_ROW mode, this is not supported.";
      CloseWithError(NotImplemented{"Single row mode is not supported"});
    case PGRES_COPY_IN:
    case PGRES_COPY_OUT:
    case PGRES_COPY_BOTH:
      LOG_ERROR() << log_extra_
                  << "PostgreSQL COPY command invoked which is not implemented"
                  << logging::LogExtra::Stacktrace();
      CloseWithError(NotImplemented{"Copy is not implemented"});
    case PGRES_BAD_RESPONSE:
      CloseWithError(ConnectionError{"Failed to parse server response"});
    case PGRES_NONFATAL_ERROR: {
      Message msg{wrapper};
      switch (msg.GetSeverity()) {
        case Message::Severity::kDebug:
          LOG_DEBUG() << log_extra_ << "Postgres " << msg.GetSeverityString()
                      << " message: " << msg.GetMessage() << msg.GetLogExtra();
          break;
        case Message::Severity::kLog:
        case Message::Severity::kInfo:
        case Message::Severity::kNotice:
          LOG_INFO() << log_extra_ << "Postgres " << msg.GetSeverityString()
                     << " message: " << msg.GetMessage() << msg.GetLogExtra();
          break;
        case Message::Severity::kWarning:
          LOG_WARNING() << log_extra_ << "Postgres " << msg.GetSeverityString()
                        << " message: " << msg.GetMessage()
                        << msg.GetLogExtra();
          break;
        case Message::Severity::kError:
        case Message::Severity::kFatal:
        case Message::Severity::kPanic:
          LOG_ERROR() << log_extra_ << "Postgres " << msg.GetSeverityString()
                      << " message (marked as non-fatal): " << msg.GetMessage()
                      << msg.GetLogExtra();
          break;
      }
      break;
    }
    case PGRES_FATAL_ERROR: {
      Message msg{wrapper};
      if (!IsWhitelistedState(msg.GetSqlState())) {
        LOG_ERROR() << log_extra_ << "Fatal error occured: " << msg.GetMessage()
                    << msg.GetLogExtra();
      } else {
        LOG_WARNING() << log_extra_
                      << "Fatal error occured: " << msg.GetMessage()
                      << msg.GetLogExtra();
      }
      msg.ThrowException();
      break;
    }
  }
  wrapper->FillBufferCategories(types);
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
