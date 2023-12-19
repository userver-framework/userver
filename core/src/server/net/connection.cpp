#include "connection.hpp"

#include <algorithm>
#include <array>
#include <stdexcept>
#include <system_error>
#include <vector>

#include <server/http/http_request_parser.hpp>
#include <server/http/request_handler_base.hpp>

#include <userver/engine/async.hpp>
#include <userver/engine/exception.hpp>
#include <userver/engine/io/exception.hpp>
#include <userver/engine/io/tls_wrapper.hpp>
#include <userver/engine/task/cancel.hpp>
#include <userver/logging/log.hpp>
#include <userver/server/request/request_config.hpp>
#include <userver/utils/assert.hpp>
#include <userver/utils/fast_scope_guard.hpp>

USERVER_NAMESPACE_BEGIN

namespace server::net {

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
      peer_name_(remote_address_.PrimaryAddressString()),
      request_tasks_(Queue::Create()) {
  LOG_DEBUG() << "Incoming connection from " << Getpeername() << ", fd "
              << Fd();

  ++stats_->active_connections;
  ++stats_->connections_created;
}

void Connection::Process() {
  LOG_TRACE() << "Starting socket listener for fd " << Fd();

  // In case of TaskProcessor overload keep receiving requests as we wish to
  // keep using the connection
  auto socket_listener = engine::CriticalAsyncNoSpan(
      [this](Queue::Producer producer, engine::TaskCancellationToken token) {
        ListenForRequests(std::move(producer), std::move(token));
      },
      request_tasks_->GetProducer(),
      engine::current_task::GetCancellationToken());

  auto consumer = request_tasks_->GetConsumer();
  ProcessResponses(consumer);

  socket_listener.SyncCancel();
  ProcessResponses(consumer);  // Consume remaining requests
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

  UASSERT(IsRequestTasksEmpty());
}

bool Connection::IsRequestTasksEmpty() const noexcept {
  return request_tasks_->GetSizeApproximate() == 0;
}

void Connection::ListenForRequests(
    Queue::Producer producer, engine::TaskCancellationToken token) noexcept {
  using RequestBasePtr = std::shared_ptr<request::RequestBase>;
  utils::FastScopeGuard send_stopper([&]() noexcept { token.RequestCancel(); });

  try {
    request_tasks_->SetSoftMaxSize(config_.requests_queue_size_threshold);

    http::HttpRequestParser request_parser(
        request_handler_.GetHandlerInfoIndex(), handler_defaults_config_,
        [this, &producer](RequestBasePtr&& request_ptr) {
          if (!NewRequest(std::move(request_ptr), producer)) {
            is_accepting_requests_ = false;
          }
        },
        stats_->parser_stats, data_accounter_);

    std::vector<char> buf(config_.in_buffer_size);
    std::size_t last_bytes_read = 0;
    while (is_accepting_requests_) {
      auto deadline = engine::Deadline::FromDuration(config_.keepalive_timeout);

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
      if (last_bytes_read != buf.size()) {
        is_readable = peer_socket_->WaitReadable(deadline);
      }

      last_bytes_read =
          is_readable ? peer_socket_->ReadSome(buf.data(), buf.size(), deadline)
                      : 0;
      if (!last_bytes_read) {
        LOG_TRACE() << "Peer " << Getpeername() << " on fd " << Fd()
                    << " closed connection or the connection timed out";

        // RFC7230 does not specify rules for connections half-closed from
        // client side. However, section 6 tells us that in most cases
        // connections are closed after sending/receiving the last response. See
        // also: https://github.com/httpwg/http-core/issues/22
        //
        // It is faster (and probably more efficient) for us to cancel currently
        // processing and pending requests.
        return;
      }
      LOG_TRACE() << "Received " << last_bytes_read << " byte(s) from "
                  << Getpeername() << " on fd " << Fd();

      if (!request_parser.Parse(buf.data(), last_bytes_read)) {
        LOG_DEBUG() << "Malformed request from " << Getpeername() << " on fd "
                    << Fd();

        // Stop accepting new requests, send previous answers.
        is_accepting_requests_ = false;
      }
    }

    send_stopper.Release();
    LOG_TRACE() << "Gracefully stopping ListenForRequests()";
  } catch (const engine::io::IoTimeout&) {
    LOG_INFO() << "Closing idle connection on timeout";
    send_stopper.Release();
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

bool Connection::NewRequest(std::shared_ptr<request::RequestBase>&& request_ptr,
                            Queue::Producer& producer) {
  if (!is_accepting_requests_) {
    /* In case of recv() of >1 requests it is possible to get here
     * after is_accepting_requests_ is set to true. Just ignore tail
     * garbage.
     */
    return true;
  }

  if (request_ptr->IsFinal()) {
    is_accepting_requests_ = false;
  }

  ++stats_->active_request_count;
  auto task = request_handler_.StartRequestTask(request_ptr);
  return producer.Push({std::move(request_ptr), std::move(task)});
}

void Connection::ProcessResponses(Queue::Consumer& consumer) noexcept {
  try {
    QueueItem item;
    while (consumer.Pop(item)) {
      HandleQueueItem(item);

      // now we must complete processing
      engine::TaskCancellationBlocker block_cancel;

      /* In stream case we don't want a user task to exit
       * until SendResponse() as the task produces body chunks.
       */
      SendResponse(*item.first);
      if (item.first->IsUpgradeWebsocket())
        item.first->DoUpgrade(std::move(peer_socket_),
                              std::move(remote_address_));
      item.first.reset();
      item.second = {};
    }
  } catch (const std::exception& e) {
    LOG_ERROR() << "Exception for fd " << Fd() << ": " << e;
  }
}

void Connection::HandleQueueItem(QueueItem& item) noexcept {
  auto& request = *item.first;

  if (engine::current_task::IsCancelRequested()) {
    // We could've packed all remaining requests into a vector and cancel them
    // in parallel. But pipelining is almost never used so why bother.
    auto request_task = std::move(item.second);
    request_task.SyncCancel();
    LOG_DEBUG() << "Request processing interrupted";
    is_response_chain_valid_ = false;
    return;  // avoids throwing and catching exception down below
  }

  try {
    auto& response = request.GetResponse();
    if (response.IsBodyStreamed()) {
      response.WaitForHeadersEnd();
    } else {
      auto request_task = std::move(item.second);
      request_task.Get();
    }
  } catch (const engine::TaskCancelledException& e) {
    LOG_LIMITED_ERROR() << "Handler task was cancelled with reason: "
                        << ToString(e.Reason());
    auto& response = request.GetResponse();
    if (!response.IsReady()) {
      response.SetReady();
      response.SetStatusServiceUnavailable();
    }
  } catch (const engine::WaitInterruptedException&) {
    LOG_DEBUG() << "Request processing interrupted";
    is_response_chain_valid_ = false;
  } catch (const std::exception& e) {
    LOG_WARNING() << "Request failed with unhandled exception: " << e;
    request.MarkAsInternalServerError();
  }
}

void Connection::SendResponse(request::RequestBase& request) {
  auto& response = request.GetResponse();
  UASSERT(!response.IsSent());
  request.SetStartSendResponseTime();
  if (is_response_chain_valid_ && peer_socket_) {
    try {
      // Might be a stream reading or a fully constructed response
      response.SendResponse(*peer_socket_);
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
  --stats_->active_request_count;
  ++stats_->requests_processed_count;

  request.WriteAccessLogs(request_handler_.LoggerAccess(),
                          request_handler_.LoggerAccessTskv(), peer_name_);
}

std::string Connection::Getpeername() const { return peer_name_; }

}  // namespace server::net

USERVER_NAMESPACE_END
