#include <storages/postgres/detail/pg_connection_wrapper.hpp>

#include <storages/postgres/dsn.hpp>
#include <storages/postgres/exceptions.hpp>
#include <storages/postgres/message.hpp>

#include <logging/log.hpp>

#define PGCW_LOG_TRACE() LOG_TRACE() << log_extra_
#define PGCW_LOG_DEBUG() LOG_DEBUG() << log_extra_
#define PGCW_LOG_INFO() LOG_INFO() << log_extra_
#define PGCW_LOG_WARNING() LOG_WARNING() << log_extra_
#define PGCW_LOG_ERROR() LOG_ERROR() << log_extra_
#define PGCW_LOG(level) LOG(level) << log_extra_

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

void NoticeReceiver(void* conn_wrapper_ptr, PGresult const* pg_res) {
  if (!conn_wrapper_ptr || !pg_res) {
    return;
  }

  auto conn_wrapper = static_cast<PGConnectionWrapper*>(conn_wrapper_ptr);
  conn_wrapper->LogNotice(pg_res);
}

}  // namespace

PGConnectionWrapper::PGConnectionWrapper(engine::TaskProcessor& tp, uint32_t id)
    : bg_task_processor_{tp} {
  // TODO add SSL initialization
  log_extra_.Extend("conn_id", id);
}

PGConnectionWrapper::~PGConnectionWrapper() { Close().Detach(); }

template <typename ExceptionType>
void PGConnectionWrapper::CheckError(const std::string& cmd,
                                     int pg_dispatch_result) {
  if (pg_dispatch_result == 0) {
    auto msg = PQerrorMessage(conn_);
    PGCW_LOG_ERROR() << "libpq " << cmd << " error: " << msg;
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

engine::Task PGConnectionWrapper::Close() {
  engine::io::Socket tmp_sock = std::exchange(socket_, {});
  PGconn* tmp_conn = std::exchange(conn_, nullptr);

  return engine::CriticalAsync(
      bg_task_processor_, [ tmp_conn, socket = std::move(tmp_sock) ]() mutable {
        if (tmp_conn != nullptr) {
          PQfinish(tmp_conn);
        }

        if (socket) {
          [[maybe_unused]] int fd = std::move(socket).Release();
        }
      });
}

template <typename ExceptionType>
void PGConnectionWrapper::CloseWithError(ExceptionType&& ex) {
  PGCW_LOG_DEBUG() << "Closing connection because of failure: " << ex.what();
  Close().Wait();
  throw std::forward<ExceptionType>(ex);
}

engine::Task PGConnectionWrapper::Cancel() {
  if (!conn_) {
    return engine::Async(bg_task_processor_, [] {});
  }
  PGCW_LOG_DEBUG() << "Cancel current request";
  std::unique_ptr<PGcancel, decltype(&PQfreeCancel)> cancel{PQgetCancel(conn_),
                                                            &PQfreeCancel};
  return engine::Async(
      bg_task_processor_, [ this, cancel = std::move(cancel) ] {
        std::array<char, kErrBufferSize> buffer;
        if (!PQcancel(cancel.get(), buffer.data(), buffer.size())) {
          PGCW_LOG_WARNING() << "Failed to cancel current request";
          // TODO Throw exception or not?
        }
      });
}

void PGConnectionWrapper::AsyncConnect(const std::string& conninfo,
                                       Duration poll_timeout) {
  PGCW_LOG_DEBUG() << "Connecting to " << DsnCutPassword(conninfo);
  StartAsyncConnect(conninfo);
  WaitConnectionFinish(Deadline::FromDuration(poll_timeout));
  PGCW_LOG_DEBUG() << "Connected to " << DsnCutPassword(conninfo);
}

void PGConnectionWrapper::StartAsyncConnect(const std::string& conninfo) {
  if (conn_) {
    PGCW_LOG_ERROR()
        << "Attempt to connect a connection that is already connected"
        << logging::LogExtra::Stacktrace();
    throw ConnectionFailed{conninfo, "Already connected"};
  }

  conn_ = PQconnectStart(conninfo.c_str());
  if (!conn_) {
    // The only reason the pointer cannot be null is that libpq failed
    // to allocate memory for the structure
    PGCW_LOG_ERROR() << "libpq failed to allocate a PGconn structure"
                     << logging::LogExtra::Stacktrace();
    throw ConnectionFailed{conninfo, "Failed to allocate PGconn structure"};
  }

  PQsetNoticeReceiver(conn_, &NoticeReceiver, this);

  if (PQsetnonblocking(conn_, 1)) {
    PGCW_LOG_ERROR() << "libpq failed to set non-blocking connection mode";
    throw ConnectionFailed{conninfo,
                           "Failed to set non-blocking connection mode"};
  }

  const auto status = PQstatus(conn_);
  const auto* msg_for_status = MsgForStatus(status);
  if (CONNECTION_BAD == status) {
    const std::string msg = msg_for_status;
    PGCW_LOG_ERROR() << msg;
    CloseWithError(ConnectionFailed{conninfo, msg});
  } else {
    PGCW_LOG_TRACE() << msg_for_status;
  }

  const auto socket = PQsocket(conn_);
  if (socket < 0) {
    PGCW_LOG_ERROR() << "Invalid PostgreSQL socket " << socket
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

void PGConnectionWrapper::WaitConnectionFinish(Deadline deadline) {
  auto poll_res = PGRES_POLLING_WRITING;
  while (poll_res != PGRES_POLLING_OK) {
    switch (poll_res) {
      case PGRES_POLLING_READING:
        if (!WaitSocketReadable(deadline)) {
          PGCW_LOG_ERROR()
              << "Timeout while polling PostgreSQL connection socket";
          throw ConnectionTimeoutError("Timed out while polling connection");
        }
        break;
      case PGRES_POLLING_WRITING:
        if (!WaitSocketWriteable(deadline)) {
          PGCW_LOG_ERROR()
              << "Timeout while polling PostgreSQL connection socket";
          throw ConnectionTimeoutError("Timed out while polling connection");
        }
        break;
      case PGRES_POLLING_ACTIVE:
        // This is an obsolete state, just ignore it
        break;
      case PGRES_POLLING_FAILED:
        PGCW_LOG_ERROR() << " libpq polling failed";
        CheckError<ConnectionError>("PQconnectPoll", 0);
      default:
        assert(!"Unexpected enumeration value");
        break;
    }
    poll_res = PQconnectPoll(conn_);
    PGCW_LOG_TRACE() << MsgForStatus(PQstatus(conn_));
  }
}

bool PGConnectionWrapper::WaitSocketReadable(Deadline deadline) {
  return socket_.WaitReadable(deadline);
}

bool PGConnectionWrapper::WaitSocketWriteable(Deadline deadline) {
  return socket_.WaitWriteable(deadline);
}

void PGConnectionWrapper::Flush(Deadline deadline) {
  while (const int flush_res = PQflush(conn_)) {
    if (flush_res < 0) {
      throw CommandError(PQerrorMessage(conn_));
    }
    if (!WaitSocketWriteable(deadline)) {
      PGCW_LOG_ERROR() << "Timeout while flushing PostgreSQL connection socket";
      throw ConnectionTimeoutError("Timed out while flushing connection");
    }
  }
}

void PGConnectionWrapper::ConsumeInput(Deadline deadline) {
  while (PQisBusy(conn_)) {
    if (!WaitSocketReadable(deadline)) {
      PGCW_LOG_ERROR()
          << "Timeout while consuming input from PostgreSQL connection socket";
      throw ConnectionTimeoutError("Timed out while consuming input");
    }
    CheckError<CommandError>("PQconsumeInput", PQconsumeInput(conn_));
  }
}

ResultSet PGConnectionWrapper::WaitResult(const UserTypes& types,
                                          Deadline deadline) {
  Flush(deadline);
  auto handle = MakeResultHandle(nullptr);
  ConsumeInput(deadline);
  while (auto pg_res = PQgetResult(conn_)) {
    if (handle) {
      // TODO Decide about the severity of this situation
      PGCW_LOG_DEBUG()
          << "Query returned several result sets, a result set is discarded";
    }
    handle = MakeResultHandle(pg_res);
    ConsumeInput(deadline);
  }
  return MakeResult(types, std::move(handle));
}

void PGConnectionWrapper::DiscardInput(Deadline deadline) {
  Flush(deadline);
  auto handle = MakeResultHandle(nullptr);
  ConsumeInput(deadline);
  while (auto pg_res = PQgetResult(conn_)) {
    handle = MakeResultHandle(pg_res);
    ConsumeInput(deadline);
  }
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
      PGCW_LOG_TRACE()
          << "Successful completion of a command returning no data";
      break;
    case PGRES_TUPLES_OK:
      PGCW_LOG_TRACE() << "Successful completion of a command returning data";
      break;
    case PGRES_SINGLE_TUPLE:
      PGCW_LOG_ERROR()
          << "libpq was switched to SINGLE_ROW mode, this is not supported.";
      CloseWithError(NotImplemented{"Single row mode is not supported"});
    case PGRES_COPY_IN:
    case PGRES_COPY_OUT:
    case PGRES_COPY_BOTH:
      PGCW_LOG_ERROR()
          << "PostgreSQL COPY command invoked which is not implemented"
          << logging::LogExtra::Stacktrace();
      CloseWithError(NotImplemented{"Copy is not implemented"});
    case PGRES_BAD_RESPONSE:
      CloseWithError(ConnectionError{"Failed to parse server response"});
    case PGRES_NONFATAL_ERROR: {
      Message msg{wrapper};
      switch (msg.GetSeverity()) {
        case Message::Severity::kDebug:
          PGCW_LOG_DEBUG() << "Postgres " << msg.GetSeverityString()
                           << " message: " << msg.GetMessage()
                           << msg.GetLogExtra();
          break;
        case Message::Severity::kLog:
        case Message::Severity::kInfo:
        case Message::Severity::kNotice:
          PGCW_LOG_INFO() << "Postgres " << msg.GetSeverityString()
                          << " message: " << msg.GetMessage()
                          << msg.GetLogExtra();
          break;
        case Message::Severity::kWarning:
          PGCW_LOG_WARNING()
              << "Postgres " << msg.GetSeverityString()
              << " message: " << msg.GetMessage() << msg.GetLogExtra();
          break;
        case Message::Severity::kError:
        case Message::Severity::kFatal:
        case Message::Severity::kPanic:
          PGCW_LOG_ERROR() << "Postgres " << msg.GetSeverityString()
                           << " message (marked as non-fatal): "
                           << msg.GetMessage() << msg.GetLogExtra();
          break;
      }
      break;
    }
    case PGRES_FATAL_ERROR: {
      Message msg{wrapper};
      if (!IsWhitelistedState(msg.GetSqlState())) {
        PGCW_LOG_ERROR() << "Fatal error occured: " << msg.GetMessage()
                         << msg.GetLogExtra();
      } else {
        PGCW_LOG_WARNING() << "Fatal error occured: " << msg.GetMessage()
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
  CheckError<CommandError>("PQsendQuery `" + statement + "`",
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

void PGConnectionWrapper::LogNotice(PGresult const* pg_res) {
  auto severity = Message::SeverityFromString(
      PQresultErrorField(pg_res, PG_DIAG_SEVERITY_NONLOCALIZED));
  auto msg = PQresultErrorMessage(pg_res);
  if (msg) {
    ::logging::Level lvl = ::logging::Level::kInfo;
    if (severity >= Message::Severity::kError) {
      lvl = ::logging::Level::kError;
    } else if (severity == Message::Severity::kWarning) {
      lvl = ::logging::Level::kWarning;
    } else if (severity < Message::Severity::kInfo) {
      lvl = ::logging::Level::kDebug;
    }
    PGCW_LOG(lvl) << msg;
  }
}

}  // namespace detail
}  // namespace postgres
}  // namespace storages
