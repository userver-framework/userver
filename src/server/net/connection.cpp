#include "connection.hpp"

#include <algorithm>
#include <array>
#include <cassert>
#include <cerrno>
#include <stdexcept>
#include <system_error>
#include <vector>

#include <boost/lexical_cast.hpp>

#include <engine/async.hpp>
#include <logging/log.hpp>
#include <server/http/http_request_handler.hpp>
#include <server/http/http_request_parser.hpp>
#include <server/request/request_config.hpp>

namespace server {
namespace net {

Connection::Connection(engine::TaskProcessor& task_processor,
                       const ConnectionConfig& config,
                       engine::io::Socket peer_socket,
                       const http::HttpRequestHandler& request_handler,
                       std::shared_ptr<Stats> stats)
    : task_processor_(task_processor),
      config_(config),
      peer_socket_(std::move(peer_socket)),
      request_handler_(request_handler),
      stats_(std::move(stats)),
      remote_address_(peer_socket_.Getpeername().RemoteAddress()),
      request_parser_(std::make_unique<http::HttpRequestParser>(
          request_handler.GetHandlerInfoIndex(), *config_.request,
          [this](std::unique_ptr<request::RequestBase>&& request_ptr) {
            NewRequest(std::move(request_ptr));
          },
          stats_->parser_stats)),
      is_accepting_requests_(true),
      stop_sender_after_queue_is_empty_(false),
      is_closing_(false) {
  LOG_DEBUG() << "Incoming connection from " << peer_socket_.Getpeername()
              << ", fd " << Fd();

  ++stats_->active_connections;
  ++stats_->connections_created;
}

Connection::~Connection() {
  const int fd = Fd();  // will be invalidated on close

  // Socket listener can be simply cancelled
  LOG_TRACE() << "Stopping socket listener for fd " << fd;
  assert(socket_listener_);
  socket_listener_ = {};
  LOG_TRACE() << "Stopped socket listener for fd " << fd;

  // SendResponses() has to wait for all responses and send them out
  LOG_TRACE() << "Waiting for request tasks queue to become empty for fd "
              << fd;
  request_tasks_empty_event_.Send();
  response_sender_task_.Wait();

  peer_socket_.Close();

  LOG_TRACE() << "RequestBase tasks queue became empty for fd " << fd;
  assert(IsRequestTasksEmpty());

  --stats_->active_connections;
  ++stats_->connections_closed;

  if (close_cb_) close_cb_();
}

void Connection::SetCloseCb(CloseCb close_cb) {
  close_cb_ = std::move(close_cb);
}

void Connection::Start() {
  shared_this_ = shared_from_this();

  LOG_TRACE() << "Starting socket listener for fd " << Fd();

  response_sender_task_ =
      engine::CriticalAsync(task_processor_, [this] { SendResponses(); });
  socket_listener_ =
      engine::CriticalAsync(task_processor_, [this] { ListenForRequests(); });
  LOG_TRACE() << "Started socket listener for fd " << Fd();
}

void Connection::Stop() {
  socket_listener_.RequestCancel();
  StopResponseSenderTaskAsync();
}

int Connection::Fd() const { return peer_socket_.Fd(); }

bool Connection::IsRequestTasksEmpty() const {
  return request_tasks_.size_approx() == 0;
}

void Connection::ListenForRequests() {
  try {
    std::vector<char> buf(config_.in_buffer_size);
    while (is_accepting_requests_) {
      size_t bytes_read = peer_socket_.RecvSome(buf.data(), buf.size(), {});
      if (!bytes_read) {
        LOG_TRACE() << "Peer " << peer_socket_.Getpeername() << " on fd "
                    << Fd() << "closed connection";
        break;
      }
      LOG_TRACE() << "Received " << bytes_read << " byte(s) from "
                  << peer_socket_.Getpeername() << " on fd " << Fd();

      if (!request_parser_->Parse(buf.data(), bytes_read)) {
        LOG_DEBUG() << "Malformed request from " << peer_socket_.Getpeername()
                    << " on fd " << Fd();
        break;
      }

      while (request_tasks_.size_approx() >=
             config_.requests_queue_size_threshold) {
        LOG_TRACE() << "Receiving from fd " << Fd()
                    << " paused due to queue overfill";
        request_task_full_event_.WaitForEvent();
      }
    }
  } catch (const std::exception& ex) {
    LOG_ERROR() << "Error while receiving from peer "
                << peer_socket_.Getpeername() << " on fd " << Fd() << ": "
                << ex.what();
  }

  StopResponseSenderTaskAsync();

  LOG_TRACE() << "Stopping ListenForRequests()";
  CloseAsync();
}

void Connection::NewRequest(
    std::unique_ptr<request::RequestBase>&& request_ptr) {
  if (!is_accepting_requests_) {
    /* In case of recv() of >1 requests it is possible to get here
     * after is_accepting_requests_ is set to true. Just ignore tail
     * garbage.
     */
    return;
  }

  if (request_ptr->IsFinal()) {
    is_accepting_requests_ = false;
  }

  ++stats_->active_request_count;
  request_tasks_.enqueue(
      request_handler_.StartRequestTask(std::move(request_ptr)));

  request_tasks_empty_event_.Send();
}

engine::TaskWithResult<std::shared_ptr<request::RequestBase>>
Connection::DequeueRequestTask() {
  engine::TaskWithResult<std::shared_ptr<request::RequestBase>> task;

  while (true) {
    if (request_tasks_.try_dequeue(task)) {
      request_task_full_event_.Send();
      break;
    }

    /* Queue is empty */

    if (stop_sender_after_queue_is_empty_) break;

    request_tasks_empty_event_.WaitForEvent();
  }

  return task;
}

void Connection::SendResponses() {
  LOG_TRACE() << "Sending responses for fd " << Fd();

  while (true) {
    auto task = DequeueRequestTask();
    if (!task) break;

    auto request_ptr = task.Get();
    auto& request = *request_ptr;
    auto& response = request.GetResponse();
    assert(!response.IsSent());
    request.SetStartSendResponseTime();
    if (peer_socket_) {
      try {
        response.SendResponse(peer_socket_);
      } catch (const std::exception& ex) {
        LOG_ERROR() << "Error sending data: " << ex.what();

        send_failure_time_ = std::chrono::steady_clock::now();
        response.SetSendFailed(send_failure_time_);
      }
    } else {
      response.SetSendFailed(send_failure_time_);
    }
    request.SetFinishSendResponseTime();
    --stats_->active_request_count;
    ++stats_->requests_processed_count;

    request.WriteAccessLogs(request_handler_.LoggerAccess(),
                            request_handler_.LoggerAccessTskv(),
                            remote_address_);
  }

  StopSocketListenersAsync();

  LOG_TRACE() << "Stopping SendResponses()";

  CloseAsync();
}

void Connection::StopSocketListenersAsync() {
  is_accepting_requests_ = false;
  request_task_full_event_.Send();
}

void Connection::StopResponseSenderTaskAsync() {
  /* Stop SendResponses() after all handlers are finished */
  stop_sender_after_queue_is_empty_ = true;
  request_tasks_empty_event_.Send();
}

void Connection::CloseAsync() {
  if (is_closing_.exchange(true)) return;

  // We should delete this, but we cannot do ~Connection() as it waits for
  // current task. So, create a mini-task for killing this.
  engine::CriticalAsync(
      [](std::shared_ptr<Connection> shared_this) { shared_this.reset(); },
      std::move(shared_this_))
      .Detach();

  // 'this' might be deleted here
}

}  // namespace net
}  // namespace server
