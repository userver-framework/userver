#include <server/net/connection_base.hpp>

#include <userver/engine/io/tls_wrapper.hpp>

#include <userver/engine/task/cancel.hpp>
#include <userver/engine/wait_any.hpp>
#include <userver/logging/log.hpp>

USERVER_NAMESPACE_BEGIN

namespace server::net {

namespace {

int ResolveFd(engine::io::RwBase& socket) noexcept {
    if (auto* raw_socket = dynamic_cast<engine::io::Socket*>(&socket)) {
        return raw_socket->Fd();
    }
    if (auto* tls_socket = dynamic_cast<engine::io::TlsWrapper*>(&socket)) {
        return tls_socket->GetRawFd();
    }
    return -2;
}

}  // namespace

ConnectionBase::ConnectionBase(
    std::unique_ptr<engine::io::RwBase> socket,
    std::string peer_name,
    const ConnectionConfig& config,
    const http::RequestHandlerBase& request_handler,
    Stats& stats
)
    : stats_(stats),
      reader_(config, std::move(peer_name)),
      socket_(std::move(socket)),
      fd_(socket_ ? ResolveFd(*socket_) : -2),
      config_(config),
      request_handler_(request_handler)
{
    UASSERT(socket_);
}

bool ConnectionBase::TryParseRequests(request::RequestParser& parser) noexcept {
    try {
        if (is_accepting_requests_) {
            if (reader_.IsEmpty() && !reader_.TryRead(GetSocket(), GetFd())) {
                // RFC7230 does not specify rules for connections half-closed from
                // client side. However, section 6 tells us that in most cases
                // connections are closed after sending/receiving the last response.
                // See also: https://github.com/httpwg/http-core/issues/22
                //
                // It is faster (and probably more efficient) for us to cancel
                // currently processing and pending requests.
                return false;
            }

            if (!parser.Parse(reader_.PeekAsStringView())) {
                LOG_DEBUG() << "Malformed request from " << GetPeerName() << " on fd " << GetFd();

                // Stop accepting new requests, send previous answers.
                is_accepting_requests_ = false;
            }
            reader_.ResetView();
            return true;
        }

        LOG_TRACE() << "Gracefully stopping ListenForRequests()";
    } catch (const engine::io::IoTimeout&) {
        LOG_INFO() << "Closing idle connection on timeout";
    } catch (const engine::io::IoCancelled&) {
        LOG_TRACE() << "engine::io::IoCancelled thrown in ListenForRequests()";
    } catch (const engine::io::IoSystemError& ex) {
        // Relying on raw error code because std::error_code comparison requires both an error code and an error
        // category, and we do not have the latter.
        auto log_level =
            ex.Code().value() == static_cast<int>(std::errc::connection_reset)
                ? logging::Level::kInfo
                : logging::Level::kError;
        LOG(log_level) << "I/O error while receiving from peer " << GetPeerName() << " on fd " << GetFd() << ": " << ex;
    } catch (const std::exception& ex) {
        LOG_ERROR() << "Error while receiving from peer " << GetPeerName() << " on fd " << GetFd() << ": " << ex;
    }
    return false;
}

void ConnectionBase::StopAcceptingRequests() noexcept { is_accepting_requests_ = false; }

bool ConnectionBase::IsResponseChainValid() const noexcept { return is_response_chain_valid_; }

int ConnectionBase::GetFd() const { return fd_; }

bool ConnectionBase::IsValid() const noexcept { return !!socket_; }

engine::io::RwBase& ConnectionBase::GetSocket() noexcept {
    UASSERT(IsValid());
    return *socket_;
}

std::unique_ptr<engine::io::RwBase> ConnectionBase::ExtractSocket() noexcept { return std::move(socket_); }

SocketBufferedReader& ConnectionBase::Reader() noexcept { return reader_; }

void ConnectionBase::Shutdown() noexcept {
    LOG_TRACE()
        << "Terminating requests processing (canceling in-flight "
           "requests) for fd "
        << GetFd();

    socket_.reset();

    --stats_.active_connections;
    ++stats_.connections_closed;
}

const std::string& ConnectionBase::GetPeerName() const noexcept { return reader_.GetPeerName(); }

bool ConnectionBase::ReadSome() noexcept {
    if (reader_.IsFull()) {
        return true;
    }
    try {
        engine::TaskCancellationBlocker blocker;
        return reader_.DoRead(GetSocket(), engine::Deadline::Passed(), GetFd()) > 0;
    } catch (const engine::io::IoTimeout&) {
        // Read only a part of SSL Record, not a EOF
    } catch (const std::exception& e) {
        LOG_WARNING() << "Exception while reading from socket: " << e;
        return false;
    }
    return true;
}

engine::TaskWithResult<void> ConnectionBase::StartRequestTask(const std::shared_ptr<http::HttpRequest>& request
) noexcept {
    return request_handler_.StartRequestTask(request);
}

engine::TaskWithResult<void> ConnectionBase::HandleQueueItem(const std::shared_ptr<http::HttpRequest>& request
) noexcept {
    auto request_task = StartRequestTask(request);

    if (engine::current_task::IsCancelRequested()) {
        // We could've packed all remaining requests into a vector and cancel them
        // in parallel. But pipelining is almost never used so why bother.
        request_task.SyncCancel();
        LOG_DEBUG() << "Request processing interrupted";
        is_response_chain_valid_ = false;
        return request_task;  // avoids throwing and catching exception down below
    }

    try {
        auto& response = request->GetHttpResponse();
        if (response.IsBodyStreamed()) {
            // TODO: wait for TCP connection closure too
            response.WaitForHeadersEnd();
        } else {
            // We must wait for one of the following events:
            // a) socket is ready - maybe it is closed and the handler task must be
            //    cancelled;
            // b) handler task is finished - the response must be written into the
            //    socket.
            // It would be wasteful to call WaitAny() each time for quick HTTP
            // handlers as it would setup-and-remove IO watcher with no real effect.
            // So avoid it for the first N microseconds; after that IO watcher
            // overhead is not too expensive compared to the await time and we can
            // tolerate its cost.

            request_task.WaitFor(config_.abort_check_delay);
            if (!request_task.IsFinished()) {
                // Slow path for not-so-fast handlers
                engine::io::ReadableBase& peer_read = GetSocket();
                const auto task_num = engine::WaitAny(peer_read, request_task);

                if (task_num == 0) {
                    if (!ReadSome()) {
                        // TCP connection is closed, cancel the user task
                        LOG_DEBUG() << "Cancelling request due to closed socket";
                        request_task.RequestCancel();
                    }
                }
            } else {
                // Fast path for quick handlers, no socket awaiting
            }
            request_task.Get();
        }
    } catch (const engine::TaskCancelledException& e) {
        auto reason = e.Reason();
        auto lvl =
            reason == engine::TaskCancellationReason::kUserRequest ? logging::Level::kWarning : logging::Level::kError;
        LOG_LIMITED(lvl) << "Handler task was cancelled with reason: " << ToString(reason);
        auto& response = request->GetHttpResponse();
        if (!response.IsReady()) {
            response.SetReady();
            response.SetStatusServiceUnavailable();
        }
    } catch (const engine::WaitInterruptedException&) {
        LOG_DEBUG() << "Request processing interrupted";
        request_task.SyncCancel();
        is_response_chain_valid_ = false;
    } catch (const std::exception& e) {
        LOG_WARNING() << "Request failed with unhandled exception: " << e;
        request->MarkAsInternalServerError();
    }
    return request_task;
}

}  // namespace server::net

USERVER_NAMESPACE_END
