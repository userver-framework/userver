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
#include <userver/engine/single_consumer_event.hpp>
#include <userver/engine/task/cancel.hpp>
#include <userver/logging/log.hpp>
#include <userver/server/request/request_config.hpp>
#include <userver/utils/assert.hpp>
#include <userver/utils/scope_guard.hpp>

USERVER_NAMESPACE_BEGIN

namespace server::net {

std::shared_ptr<Connection> Connection::Create(
    engine::TaskProcessor& task_processor, const ConnectionConfig& config,
    const request::HttpRequestConfig& handler_defaults_config,
    engine::io::Socket peer_socket,
    const http::RequestHandlerBase& request_handler,
    std::shared_ptr<Stats> stats,
    request::ResponseDataAccounter& data_accounter) {
  return std::make_shared<Connection>(
      task_processor, config, handler_defaults_config, std::move(peer_socket),
      request_handler, std::move(stats), data_accounter, EmplaceEnabler{});
}

Connection::Connection(
    engine::TaskProcessor& task_processor, const ConnectionConfig& config,
    const request::HttpRequestConfig& handler_defaults_config,
    engine::io::Socket peer_socket,
    const http::RequestHandlerBase& request_handler,
    std::shared_ptr<Stats> stats,
    request::ResponseDataAccounter& data_accounter, EmplaceEnabler)
    : task_processor_(task_processor),
      config_(config),
      handler_defaults_config_(handler_defaults_config),
      peer_socket_(std::move(peer_socket)),
      request_handler_(request_handler),
      stats_(std::move(stats)),
      data_accounter_(data_accounter),
      remote_address_(peer_socket_.Getpeername().PrimaryAddressString()),
      request_tasks_(Queue::Create()) {
  LOG_DEBUG() << "Incoming connection from " << peer_socket_.Getpeername()
              << ", fd " << Fd();

  ++stats_->active_connections;
  ++stats_->connections_created;
}

void Connection::SetCloseCb(CloseCb close_cb) {
  close_cb_ = std::move(close_cb);
}

void Connection::Start() {
  LOG_TRACE() << "Starting socket listener for fd " << Fd();

  // TODO TAXICOMMON-1993 Remove slicing once the issues with payload lifetime
  // in cancelled TaskWithResult are resolved
  engine::Task socket_listener =
      // NOLINTNEXTLINE(cppcoreguidelines-slicing)
      engine::AsyncNoSpan(
          task_processor_,
          [this](Queue::Producer producer) {
            ListenForRequests(std::move(producer));
          },
          request_tasks_->GetProducer());

  // `response_sender_task_` always starts because it is a Critical task

  // NOLINTNEXTLINE(cppcoreguidelines-slicing)
  response_sender_task_ = engine::CriticalAsyncNoSpan(
      task_processor_,
      [](std::shared_ptr<Connection> self, auto socket_listener) {
        auto consumer = self->request_tasks_->GetConsumer();
        [[maybe_unused]] bool ok =
            self->response_sender_assigned_event_.WaitForEvent();
        UASSERT(ok || engine::current_task::ShouldCancel());
        self->ProcessResponses(consumer);

        socket_listener.SyncCancel();
        self->ProcessResponses(consumer);  // Consume remaining requests
        self->Shutdown();
      },
      shared_from_this(), std::move(socket_listener));
  response_sender_launched_event_.Send();
  response_sender_assigned_event_.Send();

  LOG_TRACE() << "Started socket listener for fd " << Fd();
}

void Connection::Stop() { response_sender_task_.RequestCancel(); }

int Connection::Fd() const { return peer_socket_.Fd(); }

void Connection::Shutdown() noexcept {
  UASSERT(response_sender_task_.IsValid());

  LOG_TRACE() << "Terminating requests processing (canceling in-flight "
                 "requests) for fd "
              << Fd();

  peer_socket_.Close();  // should not throw

  --stats_->active_connections;
  ++stats_->connections_closed;

  if (close_cb_) close_cb_();  // should not throw

  UASSERT(IsRequestTasksEmpty());

  // `~Connection()` may be called from within the `response_sender_task_`.
  // Without `Detach()` we get a deadlock.
  std::move(response_sender_task_).Detach();
}

bool Connection::IsRequestTasksEmpty() const noexcept {
  return request_tasks_->GetSizeApproximate() == 0;
}

void Connection::ListenForRequests(Queue::Producer producer) noexcept {
  using RequestBasePtr = std::shared_ptr<request::RequestBase>;

  utils::ScopeGuard send_stopper([this]() {
    // do not request cancel unless we're sure it's in valid state
    // this task can only normally be cancelled from response sender
    if (response_sender_launched_event_.WaitForEvent()) {
      response_sender_task_.RequestCancel();
    }
  });

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
        is_readable = peer_socket_.WaitReadable(deadline);
      }

      last_bytes_read =
          is_readable ? peer_socket_.RecvSome(buf.data(), buf.size(), deadline)
                      : 0;
      if (!last_bytes_read) {
        LOG_TRACE() << "Peer " << peer_socket_.Getpeername() << " on fd "
                    << Fd() << " closed connection or the connection timed out";

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
                  << peer_socket_.Getpeername() << " on fd " << Fd();

      if (!request_parser.Parse(buf.data(), last_bytes_read)) {
        LOG_DEBUG() << "Malformed request from " << peer_socket_.Getpeername()
                    << " on fd " << Fd();

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
            ? logging::Level::kWarning
            : logging::Level::kError;
    LOG(log_level) << "I/O error while receiving from peer "
                   << peer_socket_.Getpeername() << " on fd " << Fd() << ": "
                   << ex;
  } catch (const std::exception& ex) {
    LOG_ERROR() << "Error while receiving from peer "
                << peer_socket_.Getpeername() << " on fd " << Fd() << ": "
                << ex;
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
  return producer.Push(
      {request_ptr, request_handler_.StartRequestTask(request_ptr)});
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
      item.first.reset();
      item.second = {};
    }
  } catch (const std::exception& e) {
    LOG_ERROR() << "Exception for fd " << Fd() << ": " << e;
  }
}

void Connection::HandleQueueItem(QueueItem& item) {
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
      response.SendResponse(peer_socket_);
    } catch (const engine::io::IoSystemError& ex) {
      // working with raw values because std::errc compares error_category
      // default_error_category() fixed only in GCC 9.1 (PR libstdc++/60555)
      auto log_level =
          ex.Code().value() == static_cast<int>(std::errc::broken_pipe)
              ? logging::Level::kWarning
              : logging::Level::kError;
      LOG(log_level) << "I/O error while sending data: " << ex;
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
                          request_handler_.LoggerAccessTskv(), remote_address_);
}

}  // namespace server::net

USERVER_NAMESPACE_END
