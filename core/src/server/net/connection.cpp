#include "connection.hpp"

#include <array>
#include <system_error>
#include <vector>

#include <server/http/http2_request_parser.hpp>
#include <server/http/http2_writer.hpp>
#include <server/http/http_request_parser.hpp>
#include <server/http/request_handler_base.hpp>

#include <userver/engine/async.hpp>
#include <userver/engine/exception.hpp>
#include <userver/engine/io/exception.hpp>
#include <userver/engine/io/tls_wrapper.hpp>
#include <userver/engine/task/cancel.hpp>
#include <userver/engine/wait_any.hpp>
#include <userver/http/common_headers.hpp>
#include <userver/http/http_version.hpp>
#include <userver/logging/log.hpp>
#include <userver/server/request/request_config.hpp>
#include <userver/utils/assert.hpp>
#include <userver/utils/fast_scope_guard.hpp>
#include <userver/utils/text_light.hpp>

USERVER_NAMESPACE_BEGIN

namespace server::net {

namespace {
constexpr std::string_view kHttp2Preface = "PRI * HTTP/2.0\r\n\r\nSM\r\n\r\n";
constexpr std::string_view kPrefaceBegin = kHttp2Preface.substr(0, 2);
}  // namespace

Connection::Connection(
    const ConnectionConfig& config,
    const request::HttpRequestConfig& handler_defaults_config,
    std::unique_ptr<engine::io::RwBase> peer_socket,
    const engine::io::Sockaddr& remote_address,
    const http::RequestHandlerBase& request_handler,
    std::shared_ptr<Stats> stats,
    request::ResponseDataAccounter& data_accounter)
    : config_(config),
      handler_defaults_config_(handler_defaults_config),
      peer_socket_(std::move(peer_socket)),
      request_handler_(request_handler),
      stats_(std::move(stats)),
      data_accounter_(data_accounter),
      remote_address_(remote_address),
      peer_name_(remote_address_.PrimaryAddressString()) {
  LOG_DEBUG() << "Incoming connection from " << Getpeername() << ", fd "
              << Fd();

  ++stats_->active_connections;
  ++stats_->connections_created;
}

void Connection::Process() {
  LOG_TRACE() << "Starting socket listener for fd " << Fd();

  ListenForRequests();

  Shutdown();
}

int Connection::Fd() const {
  auto* socket = dynamic_cast<engine::io::Socket*>(peer_socket_.get());
  if (socket) return socket->Fd();

  auto* tls_socket = dynamic_cast<engine::io::TlsWrapper*>(peer_socket_.get());
  if (tls_socket) return tls_socket->GetRawFd();

  return -2;
}

void Connection::Shutdown() noexcept {
  LOG_TRACE() << "Terminating requests processing (canceling in-flight "
                 "requests) for fd "
              << Fd();

  peer_socket_.reset();

  --stats_->active_connections;
  ++stats_->connections_closed;
}

void Connection::ListenForRequests() noexcept {
  using HttpVersion = USERVER_NAMESPACE::http::HttpVersion;
  try {
    if (GetHttpVersion() != HttpVersion::k2) {
      parser_ = MakeParser(HttpVersion::k11);
    }

    pending_data_.resize(config_.in_buffer_size);
    std::string http_version_buffer;
    http_version_buffer.reserve(kPrefaceBegin.size());
    while (is_accepting_requests_) {
      auto deadline = engine::Deadline::FromDuration(config_.keepalive_timeout);

      if (pending_data_size_ == 0) {
        bool is_readable = true;
        // If we didn't fill the buffer in the previous loop iteration we almost
        // certainly will hit EWOULDBLOCK on the subsequent recv syscall from
        // peer_socket_.RecvSome, which will fall back to event-loop waiting
        // for socket to become readable, and then issue another recv syscall,
        // effectively doing
        // 1. recv (returns -1)
        // 2. notify event-loop about read interest
        // 3. recv (return some data)
        //
        // So instead we just do 2. and 3., shaving off a whole recv syscall
        if (pending_data_size_ != pending_data_.size()) {
          is_readable = peer_socket_->WaitReadable(deadline);
        }

        pending_data_size_ =
            is_readable ? peer_socket_->ReadSome(pending_data_.data(),
                                                 pending_data_.size(), deadline)
                        : 0;
        if (!pending_data_size_) {
          LOG_TRACE() << "Peer " << Getpeername() << " on fd " << Fd()
                      << " closed connection or the connection timed out";

          // RFC7230 does not specify rules for connections half-closed from
          // client side. However, section 6 tells us that in most cases
          // connections are closed after sending/receiving the last response.
          // See also: https://github.com/httpwg/http-core/issues/22
          //
          // It is faster (and probably more efficient) for us to cancel
          // currently processing and pending requests.
          return;
        }
        LOG_TRACE() << "Received " << pending_data_size_ << " byte(s) from "
                    << Getpeername() << " on fd " << Fd();
      }

      bool should_stop_accepting_requests = false;
      bool res = false;
      const std::string_view req{pending_data_.data(), pending_data_size_};
      if (GetHttpVersion() == HttpVersion::k2) {
        if (parser_ || TryDetectHttpVersion(http_version_buffer, req)) {
          res = parser_->Parse(req);
        } else {
          // Wait next bytes to detect version
          res = true;
        }
      } else {  // Pure HTTP/1.1
        res = parser_->Parse(req);
      }
      if (!res) {
        LOG_DEBUG() << "Malformed request from " << Getpeername() << " on fd "
                    << Fd();

        // Stop accepting new requests, send previous answers.
        should_stop_accepting_requests = true;
      }
      pending_data_size_ = 0;

      for (auto&& request : pending_requests_) {
        ProcessRequest(std::move(request));
      }
      pending_requests_.resize(0);
      if (should_stop_accepting_requests) is_accepting_requests_ = false;
    }

    LOG_TRACE() << "Gracefully stopping ListenForRequests()";
  } catch (const engine::io::IoTimeout&) {
    LOG_INFO() << "Closing idle connection on timeout";
  } catch (const engine::io::IoCancelled&) {
    LOG_TRACE() << "engine::io::IoCancelled thrown in ListenForRequests()";
  } catch (const engine::io::IoSystemError& ex) {
    // working with raw values because std::errc compares error_category
    // default_error_category() fixed only in GCC 9.1 (PR libstdc++/60555)
    auto log_level =
        ex.Code().value() == static_cast<int>(std::errc::connection_reset)
            ? logging::Level::kInfo
            : logging::Level::kError;
    LOG(log_level) << "I/O error while receiving from peer " << Getpeername()
                   << " on fd " << Fd() << ": " << ex;
  } catch (const std::exception& ex) {
    LOG_ERROR() << "Error while receiving from peer " << Getpeername()
                << " on fd " << Fd() << ": " << ex;
  }
}

void Connection::ProcessRequest(
    std::shared_ptr<request::RequestBase>&& request_ptr) {
  if (request_ptr->IsFinal()) {
    is_accepting_requests_ = false;
  }

  stats_->active_request_count.Add(1);

  auto task = HandleQueueItem(request_ptr);
  SendResponse(*request_ptr);

  if (request_ptr->IsUpgradeWebsocket())
    request_ptr->DoUpgrade(std::move(peer_socket_), std::move(remote_address_));
}

bool Connection::ReadSome() {
  if (pending_data_size_ == pending_data_.size()) return true;

  try {
    engine::TaskCancellationBlocker blocker;

    auto count = peer_socket_->ReadSome(
        pending_data_.data() + pending_data_size_,
        pending_data_.size() - pending_data_size_, engine::Deadline::Passed());
    pending_data_size_ += count;
    if (count == 0) return false;
  } catch (const engine::io::IoTimeout&) {
    // Read only a part of SSL Record, not a EOF
  } catch (const std::exception& e) {
    LOG_WARNING() << "Exception while reading from socket: " << e;
    return false;
  }

  return true;
}

engine::TaskWithResult<void> Connection::HandleQueueItem(
    const std::shared_ptr<request::RequestBase>& request) noexcept {
  auto request_task = request_handler_.StartRequestTask(request);

  if (engine::current_task::IsCancelRequested()) {
    // We could've packed all remaining requests into a vector and cancel them
    // in parallel. But pipelining is almost never used so why bother.
    request_task.SyncCancel();
    LOG_DEBUG() << "Request processing interrupted";
    is_response_chain_valid_ = false;
    return request_task;  // avoids throwing and catching exception down below
  }

  try {
    auto& response = request->GetResponse();
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
        engine::io::ReadableBase& peer_read = *peer_socket_;
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
    auto lvl = reason == engine::TaskCancellationReason::kUserRequest
                   ? logging::Level::kWarning
                   : logging::Level::kError;
    LOG_LIMITED(lvl) << "Handler task was cancelled with reason: "
                     << ToString(reason);
    auto& response = request->GetResponse();
    if (!response.IsReady()) {
      response.SetReady();
      response.SetStatusServiceUnavailable();
    }
  } catch (const engine::WaitInterruptedException&) {
    LOG_DEBUG() << "Request processing interrupted";
    is_response_chain_valid_ = false;
  } catch (const std::exception& e) {
    LOG_WARNING() << "Request failed with unhandled exception: " << e;
    request->MarkAsInternalServerError();
  }
  return request_task;
}

void Connection::SendResponse(request::RequestBase& request) {
  auto& response = request.GetResponse();
  UASSERT(!response.IsSent());
  request.SetStartSendResponseTime();
  if (is_response_chain_valid_ && peer_socket_) {
    try {
      // Might be a stream reading or a fully constructed response
      if (GetHttpVersion() == USERVER_NAMESPACE::http::HttpVersion::k2) {
        // TODO: There is only one inheritor of the request::ResponseBase
        UASSERT(dynamic_cast<http::HttpResponse*>(&response));
        // NOLINTNEXTLINE(cppcoreguidelines-pro-type-static-cast-downcast)
        auto& http_response = static_cast<http::HttpResponse&>(response);
        UASSERT(dynamic_cast<http::HttpRequestImpl*>(&request));
        if (const auto& h =
                // NOLINTNEXTLINE(cppcoreguidelines-pro-type-static-cast-downcast)
            static_cast<http::HttpRequestImpl&>(request).GetHeader(
                USERVER_NAMESPACE::http::headers::k2::kHttp2SettingsHeader);
            !h.empty()) {
          auto parser = MakeParser(USERVER_NAMESPACE::http::HttpVersion::k2);
          UASSERT(dynamic_cast<http::Http2RequestParser*>(parser.get()));
          // NOLINTNEXTLINE(cppcoreguidelines-pro-type-static-cast-downcast)
          auto parser2 = static_cast<http::Http2RequestParser*>(parser.get());
          parser2->UpgradeToHttp2(h);
          // nghttp2_session_upgrade2 opens the stream with id=1 and we must use
          // it. See docs:
          // https://nghttp2.org/documentation/nghttp2_session_upgrade2.html#nghttp2-session-upgrade2
          http_response.SetStreamId(1);

          const auto send = peer_socket_->WriteAll(
              http::kSwitchingProtocolResponse.data(),
              http::kSwitchingProtocolResponse.size(), engine::Deadline{});
          LOG_TRACE() << fmt::format("Write {} bytes, and wanted write {}",
                                     send,
                                     http::kSwitchingProtocolResponse.size());
          http::WriteHttp2ResponseToSocket(*peer_socket_, http_response,
                                           *parser2);
          parser_ = std::move(parser);
          // NOLINTNEXTLINE(cppcoreguidelines-pro-type-static-cast-downcast)
        } else if (static_cast<http::HttpRequestImpl&>(request)
                       .GetHttpMajor() == 1) {
          response.SendResponse(*peer_socket_);
        } else {
          UASSERT(dynamic_cast<http::Http2RequestParser*>(parser_.get()));
          auto parser2 =
              // NOLINTNEXTLINE(cppcoreguidelines-pro-type-static-cast-downcast)
              static_cast<http::Http2RequestParser*>(parser_.get());
          http::WriteHttp2ResponseToSocket(*peer_socket_, http_response,
                                           *parser2);
        }
      } else {
        response.SendResponse(*peer_socket_);
      }
    } catch (const engine::io::IoSystemError& ex) {
      // working with raw values because std::errc compares error_category
      // default_error_category() fixed only in GCC 9.1 (PR libstdc++/60555)
      auto log_level =
          ex.Code().value() == static_cast<int>(std::errc::broken_pipe)
              ? logging::Level::kWarning
              : logging::Level::kError;
      LOG(log_level) << "I/O error while sending data: " << ex;
      response.SetSendFailed(std::chrono::steady_clock::now());
    } catch (const std::exception& ex) {
      LOG_ERROR() << "Error while sending data: " << ex;
      response.SetSendFailed(std::chrono::steady_clock::now());
    }
  } else {
    response.SetSendFailed(std::chrono::steady_clock::now());
  }
  request.SetFinishSendResponseTime();
  stats_->active_request_count.Subtract(1);
  stats_->requests_processed_count.Add(1);

  request.WriteAccessLogs(request_handler_.LoggerAccess(),
                          request_handler_.LoggerAccessTskv(), peer_name_);
}

std::string Connection::Getpeername() const { return peer_name_; }

USERVER_NAMESPACE::http::HttpVersion Connection::GetHttpVersion() const
    noexcept {
  return handler_defaults_config_.http_version;
}

std::unique_ptr<request::RequestParser> Connection::MakeParser(
    USERVER_NAMESPACE::http::HttpVersion ver) {
  const auto on_req_cb = [&pending_requests_ =
                              pending_requests_](RequestBasePtr&& request_ptr) {
    pending_requests_.push_back(std::move(request_ptr));
  };
  if (ver == USERVER_NAMESPACE::http::HttpVersion::k2) {
    return std::make_unique<http::Http2RequestParser>(
        request_handler_.GetHandlerInfoIndex(), handler_defaults_config_,
        on_req_cb, stats_->parser_stats, data_accounter_, remote_address_);
  }
  return std::make_unique<http::HttpRequestParser>(
      request_handler_.GetHandlerInfoIndex(), handler_defaults_config_,
      on_req_cb, stats_->parser_stats, data_accounter_, remote_address_);
}

bool Connection::TryDetectHttpVersion(std::string& buffer,
                                      std::string_view req) {
  using HttpVersion = USERVER_NAMESPACE::http::HttpVersion;
  const auto first_piece = buffer;
  // We can receive a 1 byte, so we have to save bytes until there are
  // enough(2 bytes) of them to detect the version
  buffer.append(req.substr(
      0, std::min(kPrefaceBegin.size() - buffer.size(), req.size())));
  if (buffer.size() > 1) {
    parser_ = (buffer == kPrefaceBegin) ? MakeParser(HttpVersion::k2)
                                        : MakeParser(HttpVersion::k11);
    UASSERT(parser_);
    if (!first_piece.empty()) {
      parser_->Parse(first_piece);
    }
  }
  return !!parser_;
}

}  // namespace server::net

USERVER_NAMESPACE_END
