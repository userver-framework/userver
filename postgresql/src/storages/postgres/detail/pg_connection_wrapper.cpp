#include <storages/postgres/detail/pg_connection_wrapper.hpp>

#include <pg_config.h>

#ifndef USERVER_NO_LIBPQ_PATCHES
#include <pq_portal_funcs.h>
#include <pq_workaround.h>
#else
auto PQXisBusy(PGconn* conn, const PGresult*) { return ::PQisBusy(conn); }
auto PQXgetResult(PGconn* conn, const PGresult*) { return ::PQgetResult(conn); }
int PQXpipelinePutSync(PGconn*) { return 0; }
auto PQXsendQueryPrepared(PGconn* conn, const char* stmtName, int nParams, const char* const* paramValues, const int* paramLengths, const int* paramFormats, int resultFormat, PGresult*) {
    return ::PQsendQueryPrepared(conn, stmtName, nParams, paramValues, paramLengths, paramFormats, resultFormat);
}
#endif

#ifdef ARCADIA_ROOT
#define USERVER_LIBPQ_VERSION PG_VERSION_NUM
#else
#include <userver_libpq_version.hpp>  // Y_IGNORE
#endif

#include <userver/concurrent/background_task_storage.hpp>
#include <userver/crypto/openssl.hpp>
#include <userver/engine/task/cancel.hpp>
#include <userver/logging/log.hpp>
#include <userver/tracing/tags.hpp>
#include <userver/utils/assert.hpp>
#include <userver/utils/strerror.hpp>

#include <storages/postgres/detail/cancel.hpp>
#include <storages/postgres/detail/pg_message_severity.hpp>
#include <storages/postgres/detail/tracing_tags.hpp>
#include <userver/storages/postgres/dsn.hpp>
#include <userver/storages/postgres/exceptions.hpp>
#include <userver/storages/postgres/io/traits.hpp>
#include <userver/storages/postgres/message.hpp>

// NOLINTNEXTLINE(cppcoreguidelines-macro-usage): uses file/line info
#define PGCW_LOG_TRACE() LOG_TRACE() << log_extra_
// NOLINTNEXTLINE(cppcoreguidelines-macro-usage): uses file/line info
#define PGCW_LOG_DEBUG() LOG_DEBUG() << log_extra_
// NOLINTNEXTLINE(cppcoreguidelines-macro-usage): uses file/line info
#define PGCW_LOG_INFO() LOG_INFO() << log_extra_
// NOLINTNEXTLINE(cppcoreguidelines-macro-usage): uses file/line info
#define PGCW_LOG_LIMITED_INFO() LOG_LIMITED_INFO() << log_extra_
// NOLINTNEXTLINE(cppcoreguidelines-macro-usage): uses file/line info
#define PGCW_LOG_WARNING() LOG_WARNING() << log_extra_
// NOLINTNEXTLINE(cppcoreguidelines-macro-usage): uses file/line info
#define PGCW_LOG_LIMITED_WARNING() LOG_LIMITED_WARNING() << log_extra_
// NOLINTNEXTLINE(cppcoreguidelines-macro-usage): uses file/line info
#define PGCW_LOG_ERROR() LOG_ERROR() << log_extra_
// NOLINTNEXTLINE(cppcoreguidelines-macro-usage): uses file/line info
#define PGCW_LOG_LIMITED_ERROR() LOG_LIMITED_ERROR() << log_extra_
// NOLINTNEXTLINE(cppcoreguidelines-macro-usage): uses file/line info
#define PGCW_LOG(level) LOG(level) << log_extra_

USERVER_NAMESPACE_BEGIN

namespace storages::postgres::detail {

std::string_view FindCommandName(std::string_view str);

namespace {

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
        case CONNECTION_GSS_STARTUP:
            return "PQstatus: Negotiating GSSAPI";
#if USERVER_LIBPQ_VERSION >= 130000
        case CONNECTION_CHECK_TARGET:
            return "PQstatus: Checking target server properties";
#endif
#if USERVER_LIBPQ_VERSION >= 140000
        case CONNECTION_CHECK_STANDBY:
            return "PQstatus: Checking if server is in standby mode";
#endif
#if USERVER_LIBPQ_VERSION >= 170000
        case CONNECTION_ALLOCATED:
            return "PQstatus: Waiting for connection attempt to be started";
#endif
    }

    UINVARIANT(false, "Unhandled ConnStatusType");
}

void NoticeReceiver(void* conn_wrapper_ptr, PGresult const* pg_res) {
    if (!conn_wrapper_ptr || !pg_res) {
        return;
    }

    auto* conn_wrapper = static_cast<PGConnectionWrapper*>(conn_wrapper_ptr);
    conn_wrapper->LogNotice(pg_res);
}

struct Openssl {
    static void Init() noexcept { [[maybe_unused]] static Openssl lock; }

private:
    Openssl() {
        // When using OpenSSL < 1.1 duplicate initialization can be problematic
        PQinitSSL(0);
        crypto::Openssl::Init();
    }
};

}  // namespace

PGConnectionWrapper::PGConnectionWrapper(
    engine::TaskProcessor& tp,
    concurrent::BackgroundTaskStorageCore& bts,
    uint32_t id,
    engine::SemaphoreLock&& pool_size_lock
)
    : bg_task_processor_{tp},
      bg_task_storage_{bts},
      log_extra_{{tracing::kDatabaseType, tracing::kDatabasePostgresType}, {"pg_conn_id", id}},
      pool_size_lock_{std::move(pool_size_lock)},
      last_use_{std::chrono::steady_clock::now()} {
    Openssl::Init();
}

PGConnectionWrapper::~PGConnectionWrapper() { bg_task_storage_.Detach(Close()); }

template <typename ExceptionType>
void PGConnectionWrapper::CheckError(const std::string& cmd, int pg_dispatch_result) {
    static constexpr std::string_view kCheckConnectionQuota =
        ". It may be useful to check the user's connection quota in the cloud "
        "or the server configuration";

    if (pg_dispatch_result == 0) {
        HandleSocketPostClose();
        auto* msg = PQerrorMessage(conn_);
        PGCW_LOG_WARNING() << "libpq " << cmd << " error: " << msg
                           << (std::is_base_of_v<ConnectionError, ExceptionType> ? kCheckConnectionQuota : "");
        throw ExceptionType(cmd + " execution error: " + msg);
    }
}

void PGConnectionWrapper::HandleSocketPostClose() {
    auto fd = PQsocket(conn_);
    if (fd < 0) {
        /*
         * Don't use the result, fd is invalid anyway. As fd is invalid,
         * we have to guarantee that we are not waiting for it.
         */
        auto fd_invalid = std::move(socket_).Release();
        if (fd_invalid >= 0) {
            LOG_DEBUG() << "Socket is closed by libpq";
        }
    }
}

PGTransactionStatusType PGConnectionWrapper::GetTransactionStatus() const { return PQtransactionStatus(conn_); }

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

    UINVARIANT(false, "Unexpected transaction status");
}

int PGConnectionWrapper::GetServerVersion() const { return PQserverVersion(conn_); }

std::string_view PGConnectionWrapper::GetParameterStatus(const char* name) const {
    const char* value = PQparameterStatus(conn_, name);
    if (!value) return {};
    return value;
}

engine::Task PGConnectionWrapper::Close() {
    engine::io::Socket tmp_sock = std::exchange(socket_, {});
    PGconn* tmp_conn = std::exchange(conn_, nullptr);

    // NOLINTNEXTLINE(cppcoreguidelines-slicing)
    return engine::CriticalAsyncNoSpan(
        bg_task_processor_,
        [tmp_conn, socket = std::move(tmp_sock), is_broken = is_broken_, sl = std::move(pool_size_lock_)]() mutable {
            int fd = -1;
            if (socket) {
                fd = std::move(socket).Release();
            }

            // We must call `PQfinish` only after we do the `socket.Release()`.
            // Otherwise `PQfinish` closes the file descriptor fd but we keep
            // listening for that fd in the `socket` variable, and if `fd` number is
            // reused we may accidentally receive alien events.

            if (tmp_conn != nullptr) {
                if (is_broken) {
                    int pq_fd = PQsocket(tmp_conn);
                    if (fd != -1 && pq_fd != -1 && fd != pq_fd) {
                        LOG_LIMITED_ERROR() << "fd from socket != fd from PQsocket (" << fd << " != " << pq_fd << ')';
                    }
                    if (pq_fd >= 0) {
                        int res = shutdown(pq_fd, SHUT_RDWR);
                        if (res < 0) {
                            auto old_errno = errno;
                            LOG_WARNING() << "error while shutdown() socket (" << old_errno
                                          << "): " << USERVER_NAMESPACE::utils::strerror(old_errno);
                        }
                    }
                }
                PQfinish(tmp_conn);
            }
        }
    );
}

template <typename ExceptionType>
void PGConnectionWrapper::CloseWithError(ExceptionType&& ex) {
    PGCW_LOG_DEBUG() << "Closing connection because of failure: " << ex;
    Close().Wait();
    throw std::forward<ExceptionType>(ex);
}

engine::Task PGConnectionWrapper::Cancel() {
    if (!conn_) {
        // NOLINTNEXTLINE(cppcoreguidelines-slicing)
        return engine::AsyncNoSpan(bg_task_processor_, [] {});
    }
    PGCW_LOG_DEBUG() << "Cancel current request";
    std::unique_ptr<PGcancel, decltype(&PQfreeCancel)> cancel{PQgetCancel(conn_), &PQfreeCancel};

    // NOLINTNEXTLINE(cppcoreguidelines-slicing)
    return engine::AsyncNoSpan([this, cancel = std::move(cancel)] {
        try {
            detail::Cancel(cancel.get(), engine::Deadline::FromDuration(std::chrono::seconds(5)));
        } catch (const std::exception& e) {
            PGCW_LOG_LIMITED_WARNING() << "Failed to cancel current request";
        }
    });
}

void PGConnectionWrapper::AsyncConnect(const Dsn& dsn, Deadline deadline, tracing::ScopeTime& scope) {
    PGCW_LOG_DEBUG() << "Connecting to " << DsnCutPassword(dsn);

    auto options = OptionsFromDsn(dsn);
    log_extra_.Extend(tracing::kDatabaseInstance, std::move(options.dbname));
    log_extra_.Extend(tracing::kPeerAddress, std::move(options.host) + ':' + options.port);

    scope.Reset(scopes::kLibpqConnect);
    StartAsyncConnect(dsn);
    scope.Reset(scopes::kLibpqWaitConnectFinish);
    WaitConnectionFinish(deadline, dsn);
    PGCW_LOG_DEBUG() << "Connected to " << DsnCutPassword(dsn);
}

void PGConnectionWrapper::StartAsyncConnect(const Dsn& dsn) {
    if (conn_) {
        PGCW_LOG_LIMITED_ERROR() << "Attempt to connect a connection that is already connected"
                                 << logging::LogExtra::Stacktrace();
        throw ConnectionFailed{dsn, "Already connected"};
    }

    // PQconnectStart() may access /etc/hosts, ~/.pgpass, /etc/passwd, etc.
    engine::CriticalAsyncNoSpan(bg_task_processor_, [&dsn, this] {
        conn_ = PQconnectStart(dsn.GetUnderlying().c_str());
    }).Get();

    if (!conn_) {
        // The only reason the pointer cannot be null is that libpq failed
        // to allocate memory for the structure
        PGCW_LOG_LIMITED_ERROR() << "libpq failed to allocate a PGconn structure" << logging::LogExtra::Stacktrace();
        throw ConnectionFailed{dsn, "Failed to allocate PGconn structure"};
    }

    const auto status = PQstatus(conn_);
    if (CONNECTION_BAD == status) {
        const std::string msg = MsgForStatus(status);
        PGCW_LOG_WARNING() << msg;
        CloseWithError(ConnectionFailed{dsn, msg});
    } else {
        PGCW_LOG_TRACE() << MsgForStatus(status);
    }

    RefreshSocket(dsn);

    // set this as early as possible to avoid dumping notices to stderr
    PQsetNoticeReceiver(conn_, &NoticeReceiver, this);

    if (kVerboseErrors) {
        PQsetErrorVerbosity(conn_, PQERRORS_VERBOSE);
    }
}

void PGConnectionWrapper::WaitConnectionFinish(Deadline deadline, const Dsn& dsn) {
    auto poll_res = PGRES_POLLING_WRITING;
    auto timeout = std::chrono::duration_cast<std::chrono::milliseconds>(deadline.TimeLeft());
    while (poll_res != PGRES_POLLING_OK) {
        switch (poll_res) {
            case PGRES_POLLING_READING:
                if (!WaitSocketReadable(deadline)) {
                    if (engine::current_task::ShouldCancel()) {
                        throw ConnectionInterrupted("Task cancelled while polling connection for reading");
                    }
                    PGCW_LOG_LIMITED_WARNING() << "Timeout while polling PostgreSQL connection "
                                                  "socket for reading, timeout was "
                                               << timeout.count() << "ms";
                    throw ConnectionTimeoutError("Timed out while polling connection for reading");
                }
                break;
            case PGRES_POLLING_WRITING:
                if (!WaitSocketWriteable(deadline)) {
                    if (engine::current_task::ShouldCancel()) {
                        throw ConnectionInterrupted("Task cancelled while polling connection for writing");
                    }
                    PGCW_LOG_LIMITED_WARNING() << "Timeout while polling PostgreSQL connection "
                                                  "socket for writing, timeout was "
                                               << timeout.count() << "ms";
                    throw ConnectionTimeoutError("Timed out while polling connection for writing");
                }
                break;
            case PGRES_POLLING_ACTIVE:
                // This is an obsolete state, just ignore it
                break;
            case PGRES_POLLING_FAILED:
                // Handled right after PQconnectPoll
            default:
                UASSERT(!"Unexpected enumeration value");
                break;
        }

        // PQconnectPoll() may access /tmp/krb5cc* files
        poll_res = engine::CriticalAsyncNoSpan(bg_task_processor_, [this] { return PQconnectPoll(conn_); }).Get();

        // Handled here so as not to loose the error message
        if (poll_res == PGRES_POLLING_FAILED) {
            CheckError<ConnectionError>("PQconnectPoll", 0);
        }

        PGCW_LOG_TRACE() << MsgForStatus(PQstatus(conn_));

        // Libpq may reopen sockets during PQconnectPoll while trying different
        // security/encryption schemes (SSL, GSS etc.). We must keep track of the
        // current socket to avoid polling the wrong one in the future.
        RefreshSocket(dsn);
    }

    // fe-exec.c: Needs to be called only on a connected database connection.
    if (PQsetnonblocking(conn_, 1)) {
        HandleSocketPostClose();
        PGCW_LOG_LIMITED_ERROR() << "libpq failed to set non-blocking connection mode";
        throw ConnectionFailed{dsn, "Failed to set non-blocking connection mode"};
    }
}

void PGConnectionWrapper::EnterPipelineMode() {
#if LIBPQ_HAS_PIPELINING
    if (!PQenterPipelineMode(conn_)) {
        PGCW_LOG_LIMITED_ERROR() << "libpq failed to enter pipeline connection mode";
        throw ConnectionError{"Failed to enter pipeline connection mode"};
    }
    PGCW_LOG_INFO() << "Entered pipeline mode";
#else
    UINVARIANT(false, "Pipeline mode is not supported");
#endif
}

void PGConnectionWrapper::ExitPipelineMode() {
#if LIBPQ_HAS_PIPELINING
    if (!PQexitPipelineMode(conn_)) {
        PGCW_LOG_LIMITED_ERROR() << "libpq failed to exit pipeline connection mode";
        throw ConnectionError{"Failed to exit pipeline connection mode"};
    }
#else
    UINVARIANT(false, "Pipeline mode is not supported");
#endif
}

bool PGConnectionWrapper::IsSyncingPipeline() const { return pipeline_sync_counter_ > 0; }

bool PGConnectionWrapper::IsPipelineActive() const {
#if LIBPQ_HAS_PIPELINING
    return PQpipelineStatus(conn_) != PQ_PIPELINE_OFF;
#else
    return false;
#endif
}

void PGConnectionWrapper::RefreshSocket(const Dsn& dsn) {
    const auto fd = PQsocket(conn_);
    if (fd < 0) {
        PGCW_LOG_LIMITED_ERROR() << "Invalid PostgreSQL socket " << fd;
        throw ConnectionFailed{dsn, "Invalid socket handle"};
    }
    if (fd == socket_.Fd()) return;

    if (socket_.IsValid()) {
        auto old_fd = std::move(socket_).Release();
        PGCW_LOG_DEBUG() << "Released abandoned PostgreSQL socket " << old_fd;
    }
    socket_ = engine::io::Socket(fd);
}

bool PGConnectionWrapper::WaitSocketReadable(Deadline deadline) {
    if (!socket_.IsValid()) return false;
    return socket_.WaitReadable(deadline);
}

bool PGConnectionWrapper::WaitSocketWriteable(Deadline deadline) {
    if (!socket_.IsValid()) return false;
    return socket_.WaitWriteable(deadline);
}

void PGConnectionWrapper::Flush(Deadline deadline) {
#if LIBPQ_HAS_PIPELINING
    if (PQpipelineStatus(conn_) != PQ_PIPELINE_OFF) {
        HandleSocketPostClose();
        CheckError<CommandError>("PQpipelineSync", PQpipelineSync(conn_));
        ++pipeline_sync_counter_;
    }
#endif
    while (const int flush_res = PQflush(conn_)) {
        if (flush_res < 0) {
            HandleSocketPostClose();
            throw CommandError(PQerrorMessage(conn_));
        }
        if (!WaitSocketWriteable(deadline)) {
            if (engine::current_task::ShouldCancel()) {
                throw ConnectionInterrupted("Task cancelled while flushing connection");
            }
            PGCW_LOG_LIMITED_WARNING() << "Timeout while flushing PostgreSQL connection socket";
            throw ConnectionTimeoutError("Timed out while flushing connection");
        }
        UpdateLastUse();
    }
}

void PGConnectionWrapper::HandlePipelineSync() {
    if (!pipeline_sync_counter_) {
        MarkAsBroken();
        throw LogicError{"Pipeline out of sync"};
    }
    --pipeline_sync_counter_;
}

bool PGConnectionWrapper::TryConsumeInput(Deadline deadline, const PGresult* description) {
    while (PQXisBusy(conn_, description)) {
        HandleSocketPostClose();
        if (!WaitSocketReadable(deadline)) {
            LOG_DEBUG() << "Socket " << socket_.Fd() << " has not become readable in TryConsumeInput due to "
                        << (socket_.IsValid() ? "timeout" : "closed fd");
            return false;
        }
        CheckError<CommandError>("PQconsumeInput", PQconsumeInput(conn_));
        UpdateLastUse();
    }
    return true;
}

void PGConnectionWrapper::ConsumeInput(Deadline deadline, const PGresult* description) {
    if (!TryConsumeInput(deadline, description)) {
        std::string additional_info;
        if (description) {
            // NOLINTNEXTLINE(cppcoreguidelines-pro-type-const-cast)
            const auto command_name = FindCommandName(PQcmdStatus(const_cast<PGresult*>(description)));
            if (!command_name.empty()) {
                additional_info = fmt::format(" for '{}' command", command_name);
            }
        }

        if (engine::current_task::ShouldCancel()) {
            throw ConnectionInterrupted("Task cancelled while consuming input" + additional_info);
        }

        auto message = "Timeout while consuming input from PostgreSQL connection" + std::move(additional_info);
        PGCW_LOG_LIMITED_WARNING() << message;
        throw ConnectionTimeoutError(std::move(message));
    }
}

ResultSet PGConnectionWrapper::WaitResult(Deadline deadline, tracing::ScopeTime& scope, const PGresult* description) {
    scope.Reset(scopes::kLibpqWaitResult);
    Flush(deadline);
    auto handle = MakeResultHandle(nullptr);
    auto null_res_counter{0};
    do {
        while (auto* pg_res = ReadResult(deadline, description)) {
            null_res_counter = 0;
            if (handle && !pipeline_sync_counter_) {
                // TODO Decide about the severity of this situation
                PGCW_LOG_LIMITED_INFO() << "Query returned several result sets, a result set is discarded";
            }
            auto next_handle = MakeResultHandle(pg_res);
#if LIBPQ_HAS_PIPELINING
            const auto status = PQresultStatus(pg_res);
            if (status == PGRES_PIPELINE_SYNC)
                HandlePipelineSync();
            else if (status != PGRES_PIPELINE_ABORTED)
#endif
                handle = std::move(next_handle);
        }
        // There is an issue with libpq when db shuts down we may receive an error
        // instead of PGRES_PIPELINE_SYNC and never get out of this cycle, hence
        // this counter.
        if (++null_res_counter > 2) {
            MarkAsBroken();
            if (!handle) throw RuntimeError{"Empty result"};
            pipeline_sync_counter_ = 0;
        }
    } while (IsSyncingPipeline() && PQstatus(conn_) != CONNECTION_BAD);

    return MakeResult(std::move(handle));
}

Notification PGConnectionWrapper::WaitNotify(Deadline deadline) {
    auto notify = std::unique_ptr<PGnotify, decltype(&PQfreemem)>(PQnotifies(conn_), &PQfreemem);
    while (!notify) {
        if (!WaitSocketReadable(deadline)) {
            throw ConnectionTimeoutError("Socket has not become readable in WaitNotify");
        }
        CheckError<CommandError>("PQconsumeInput", PQconsumeInput(conn_));
        UpdateLastUse();
        notify.reset(PQnotifies(conn_));
    }
    Notification result;
    result.channel = notify->relname;
    if (*notify->extra) result.payload = notify->extra;

    return result;
}

std::vector<ResultSet> PGConnectionWrapper::GatherPipeline(
    [[maybe_unused]] Deadline deadline,
    const std::vector<const PGresult*>& descriptions
) {
    UASSERT(!descriptions.empty());

#if !LIBPQ_HAS_PIPELINING
    UINVARIANT(false, "QueryQueue usage requires pipelining to be enabled");
#else
    Flush(deadline);

    std::vector<ResultSet> result{};
    const PGresult* current_description = descriptions.front();

    std::size_t null_res_counter{0};
    while (IsSyncingPipeline() && PQstatus(conn_) != CONNECTION_BAD) {
        auto handle = MakeResultHandle(nullptr);
        while (auto* pg_res = ReadResult(deadline, current_description)) {
            null_res_counter = 0;
            auto next_handle = MakeResultHandle(pg_res);

            const auto status = PQresultStatus(pg_res);
            if (status == PGRES_PIPELINE_SYNC) {
                HandlePipelineSync();
            } else if (status != PGRES_PIPELINE_ABORTED) {
                handle = std::move(next_handle);
            }
        }

        if (++null_res_counter > 2) {
            MarkAsBroken();
            if (!handle) {
                throw RuntimeError{"Empty result"};
            }
            pipeline_sync_counter_ = 0;
        }

        if (handle != nullptr) {
            // We might have 'SELECT set_config(...)' into pipeline, which comes from
            // SetStatementTimeout. We don't need that into our results, and this
            // seems to be the best way to distinguish from actual results.
            const auto is_set_config_response = [&handle] {
                const auto* first_field_name = PQfname(handle.get(), 0);
                return first_field_name != nullptr && std::string_view{first_field_name} == kSetConfigQueryResultName;
            }();
            if (!is_set_config_response) {
                result.push_back(MakeResult(std::move(handle)));
            }
        }

        // If for some reason there are more results than descriptions, we will
        // error out because of the description being nullptr.
        //
        // We do it this way instead of 1:1 matching because we need to feed
        // something into the last ReadResult call, which is expected to just return
        // null right away. And if it doesn't -- we get an error, as we should.
        current_description = result.size() < descriptions.size() ? descriptions[result.size()] : nullptr;
    }

    return result;
#endif
}

void PGConnectionWrapper::DiscardInput(Deadline deadline) {
    Flush(deadline);
    auto handle = MakeResultHandle(nullptr);
    auto null_res_counter{0};
    do {
        while (auto* pg_res = ReadResult(deadline, nullptr)) {
            null_res_counter = 0;
            handle = MakeResultHandle(pg_res);
#if LIBPQ_HAS_PIPELINING
            if (PQresultStatus(pg_res) == PGRES_PIPELINE_SYNC) {
                HandlePipelineSync();
            }
#endif
        }
        // Same issue as with WaitResult
        if (++null_res_counter > 2) {
            MarkAsBroken();
            pipeline_sync_counter_ = 0;
        }
    } while (IsSyncingPipeline() && PQstatus(conn_) != CONNECTION_BAD);
}

void PGConnectionWrapper::FillSpanTags(tracing::Span& span, const CommandControl& cc) const {
    // With inheritable tags, they would end up being duplicated in current Span
    // and in log_extra_ (passed by PGCW_LOG_ macros).
    span.AddNonInheritableTags(log_extra_);
    span.AddTag("network_timeout_ms", cc.execute.count());
    span.AddTag("statement_timeout_ms", cc.statement.count());
}

PGresult* PGConnectionWrapper::ReadResult(Deadline deadline, const PGresult* description) {
    ConsumeInput(deadline, description);
    return PQXgetResult(conn_, description);
}

ResultSet PGConnectionWrapper::MakeResult(ResultHandle&& handle) {
    if (!handle) {
        LOG_DEBUG() << "Empty result";
        return ResultSet{nullptr};
    }

    auto wrapper = std::make_shared<detail::ResultWrapper>(std::move(handle));
    auto status = wrapper->GetStatus();
    switch (status) {
        case PGRES_EMPTY_QUERY:
            throw LogicError{"Empty query"};
        case PGRES_COMMAND_OK:
            PGCW_LOG_TRACE() << "Successful completion of a command returning no data";
            break;
        case PGRES_TUPLES_OK:
            PGCW_LOG_TRACE() << "Successful completion of a command returning data";
            break;
        case PGRES_SINGLE_TUPLE:
            PGCW_LOG_LIMITED_ERROR() << "libpq was switched to SINGLE_ROW mode, this is not supported.";
            CloseWithError(NotImplemented{"Single row mode is not supported"});
        case PGRES_COPY_IN:
        case PGRES_COPY_OUT:
        case PGRES_COPY_BOTH:
            PGCW_LOG_LIMITED_ERROR() << "PostgreSQL COPY command invoked which is not implemented"
                                     << logging::LogExtra::Stacktrace();
            CloseWithError(NotImplemented{"Copy is not implemented"});
        case PGRES_BAD_RESPONSE:
            CloseWithError(ConnectionError{"Failed to parse server response"});
        case PGRES_NONFATAL_ERROR: {
            Message msg{wrapper};
            switch (msg.GetSeverity()) {
                case Message::Severity::kDebug:
                    PGCW_LOG_DEBUG() << "Postgres " << msg.GetSeverityString() << " message: " << msg.GetMessage()
                                     << msg.GetLogExtra();
                    break;
                case Message::Severity::kLog:
                case Message::Severity::kInfo:
                case Message::Severity::kNotice:
                    PGCW_LOG_INFO() << "Postgres " << msg.GetSeverityString() << " message: " << msg.GetMessage()
                                    << msg.GetLogExtra();
                    break;
                case Message::Severity::kWarning:
                    PGCW_LOG_LIMITED_WARNING() << "Postgres " << msg.GetSeverityString()
                                               << " message: " << msg.GetMessage() << msg.GetLogExtra();
                    break;
                case Message::Severity::kError:
                case Message::Severity::kFatal:
                case Message::Severity::kPanic:
                    PGCW_LOG_LIMITED_WARNING()
                        << "Postgres " << msg.GetSeverityString()
                        << " message (marked as non-fatal): " << msg.GetMessage() << msg.GetLogExtra();
                    break;
            }
            break;
        }
        case PGRES_FATAL_ERROR: {
            Message msg{wrapper};
            if (!IsWhitelistedState(msg.GetSqlState())) {
                PGCW_LOG_LIMITED_ERROR() << "Fatal error occurred: " << msg.GetMessage() << msg.GetLogExtra();
            } else {
                PGCW_LOG_LIMITED_WARNING() << "Fatal error occurred: " << msg.GetMessage() << msg.GetLogExtra();
            }
            LOG_DEBUG() << "Ready to throw";
            msg.ThrowException();
            break;
        }
#if USERVER_LIBPQ_VERSION >= 140000
        case PGRES_PIPELINE_ABORTED:
            PGCW_LOG_LIMITED_WARNING() << "Command failure in a pipeline";
            CloseWithError(ConnectionError{"Command failure in a pipeline"});
            break;
        case PGRES_PIPELINE_SYNC:
            PGCW_LOG_TRACE() << "Successful completion of all commands in a pipeline";
            break;
#endif
#if USERVER_LIBPQ_VERSION >= 170000
        case PGRES_TUPLES_CHUNK:
            PGCW_LOG_LIMITED_WARNING() << "Got a chunk of tuples from the larger resultset";
            CloseWithError(ConnectionError{"Got a chunk of tuples from the larger resultset"});
            break;
#endif
    }
    LOG_DEBUG() << "Result checked";
    return ResultSet{wrapper};
}

void PGConnectionWrapper::SendQuery(const std::string& statement, tracing::ScopeTime& scope) {
    scope.Reset(scopes::kLibpqSendQueryParams);
    CheckError<CommandError>(
        "PQsendQueryParams",
        PQsendQueryParams(conn_, statement.c_str(), 0, nullptr, nullptr, nullptr, nullptr, io::kPgBinaryDataFormat)
    );
    UpdateLastUse();
}

void PGConnectionWrapper::SendQuery(
    const std::string& statement,
    const QueryParameters& params,
    tracing::ScopeTime& scope
) {
    if (params.Empty()) {
        SendQuery(statement, scope);
        return;
    }
    scope.Reset(scopes::kLibpqSendQueryParams);
    CheckError<CommandError>(
        "PQsendQueryParams",
        PQsendQueryParams(
            conn_,
            statement.c_str(),
            params.Size(),
            params.ParamTypesBuffer(),
            params.ParamBuffers(),
            params.ParamLengthsBuffer(),
            params.ParamFormatsBuffer(),
            io::kPgBinaryDataFormat
        )
    );
    UpdateLastUse();
}

void PGConnectionWrapper::SendPrepare(
    const std::string& name,
    const std::string& statement,
    const QueryParameters& params,
    tracing::ScopeTime& scope
) {
    scope.Reset(scopes::kLibpqSendPrepare);
    if (params.Empty()) {
        CheckError<CommandError>("PQsendPrepare", PQsendPrepare(conn_, name.c_str(), statement.c_str(), 0, nullptr));
    } else {
        CheckError<CommandError>(
            "PQsendPrepare",
            PQsendPrepare(conn_, name.c_str(), statement.c_str(), params.Size(), params.ParamTypesBuffer())
        );
    }
    UpdateLastUse();
}

void PGConnectionWrapper::SendDescribePrepared(const std::string& name, tracing::ScopeTime& scope) {
    scope.Reset(scopes::kLibpqSendDescribePrepared);
    CheckError<CommandError>("PQsendDescribePrepared", PQsendDescribePrepared(conn_, name.c_str()));
    UpdateLastUse();
}

void PGConnectionWrapper::SendPreparedQuery(
    const std::string& name,
    const QueryParameters& params,
    tracing::ScopeTime& scope,
    PGresult* description
) {
    const auto empty = params.Empty();

    const auto size = params.Size();
    const auto* param_values = empty ? nullptr : params.ParamBuffers();
    const auto* param_lengths = empty ? nullptr : params.ParamLengthsBuffer();
    const auto* param_formats = empty ? nullptr : params.ParamFormatsBuffer();

    scope.Reset(scopes::kLibpqSendQueryPrepared);
    if (description) {
        CheckError<CommandError>(
            "PQXsendQueryPrepared",
            PQXsendQueryPrepared(
                conn_,
                name.c_str(),
                size,
                param_values,
                param_lengths,
                param_formats,
                io::kPgBinaryDataFormat,
                description
            )
        );
    } else {
        CheckError<CommandError>(
            "PQsendQueryPrepared",
            PQsendQueryPrepared(
                conn_, name.c_str(), size, param_values, param_lengths, param_formats, io::kPgBinaryDataFormat
            )
        );
    }
    UpdateLastUse();
}

#ifndef USERVER_NO_LIBPQ_PATCHES
void PGConnectionWrapper::SendPortalBind(
    const std::string& statement_name,
    const std::string& portal_name,
    const QueryParameters& params,
    tracing::ScopeTime& scope
) {
    scope.Reset(scopes::kPqSendPortalBind);
    if (params.Empty()) {
        CheckError<CommandError>(
            "PQXSendPortalBind",
            PQXSendPortalBind(
                conn_,
                statement_name.c_str(),
                portal_name.c_str(),
                0,
                nullptr,
                nullptr,
                nullptr,
                io::kPgBinaryDataFormat
            )
        );
    } else {
        CheckError<CommandError>(
            "PQXSendPortalBind",
            PQXSendPortalBind(
                conn_,
                statement_name.c_str(),
                portal_name.c_str(),
                params.Size(),
                params.ParamBuffers(),
                params.ParamLengthsBuffer(),
                params.ParamFormatsBuffer(),
                io::kPgBinaryDataFormat
            )
        );
    }
    UpdateLastUse();
}

void PGConnectionWrapper::SendPortalExecute(
    const std::string& portal_name,
    std::uint32_t n_rows,
    tracing::ScopeTime& scope
) {
    scope.Reset(scopes::kPqSendPortalExecute);
    CheckError<CommandError>("PQXSendPortalExecute", PQXSendPortalExecute(conn_, portal_name.c_str(), n_rows));
    UpdateLastUse();
}
#else
void PGConnectionWrapper::
    SendPortalBind(const std::string&, const std::string&, const QueryParameters&, tracing::ScopeTime&) {
    UINVARIANT(false, "Portals are disabled by CMake option USERVER_FEATURE_PATCH_LIBPQ");
}

void PGConnectionWrapper::SendPortalExecute(const std::string&, std::uint32_t, tracing::ScopeTime&) {
    UINVARIANT(false, "Portals are disabled by CMake option USERVER_FEATURE_PATCH_LIBPQ");
}

#endif

void PGConnectionWrapper::LogNotice(const PGresult* pg_res) const {
    const auto severity = Message::SeverityFromString(GetMachineReadableSeverity(pg_res));

    auto* msg = PQresultErrorMessage(pg_res);
    if (msg) {
        logging::Level lvl = logging::Level::kInfo;
        if (severity >= Message::Severity::kError) {
            lvl = logging::Level::kError;
        } else if (severity == Message::Severity::kWarning) {
            lvl = logging::Level::kWarning;
        } else if (severity < Message::Severity::kInfo) {
            lvl = logging::Level::kDebug;
        }
        PGCW_LOG(lvl) << msg;
    }
}

void PGConnectionWrapper::UpdateLastUse() { last_use_ = std::chrono::steady_clock::now(); }

TimeoutDuration PGConnectionWrapper::GetIdleDuration() const {
    return std::chrono::duration_cast<TimeoutDuration>(std::chrono::steady_clock::now() - last_use_);
}

void PGConnectionWrapper::MarkAsBroken() { is_broken_ = true; }

bool PGConnectionWrapper::IsBroken() const { return is_broken_; }

bool PGConnectionWrapper::IsInAbortedPipeline() const {
#if LIBPQ_HAS_PIPELINING
    return PQpipelineStatus(conn_) == PGpipelineStatus::PQ_PIPELINE_ABORTED;
#else
    return false;
#endif
}

std::string PGConnectionWrapper::EscapeIdentifier(std::string_view str) {
    auto result =
        std::unique_ptr<char, decltype(&PQfreemem)>(PQescapeIdentifier(conn_, str.data(), str.length()), &PQfreemem);
    if (!result) {
        throw CommandError{fmt::format("PQescapeIdentifier error: ", PQerrorMessage(conn_))};
    }
    return {result.get()};
}

void PGConnectionWrapper::PutPipelineSync() {
#if !LIBPQ_HAS_PIPELINING
    UINVARIANT(false, "Pipeline mode is not supported");
#else
    if (PQpipelineStatus(conn_) != PQ_PIPELINE_OFF) {
        HandleSocketPostClose();
        CheckError<CommandError>("PQXpipelinePutSync", PQXpipelinePutSync(conn_));
        ++pipeline_sync_counter_;
    }
#endif
}

}  // namespace storages::postgres::detail

USERVER_NAMESPACE_END
