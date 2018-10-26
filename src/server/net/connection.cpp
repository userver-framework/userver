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
                       engine::io::Socket peer_socket, Type type,
                       const http::HttpRequestHandler& request_handler,
                       BeforeCloseCb before_close_cb)
    : task_processor_(task_processor),
      config_(config),
      peer_socket_(std::move(peer_socket)),
      type_(type),
      request_handler_(request_handler),
      remote_address_(peer_socket_.Getpeername().RemoteAddress()),
      request_tasks_sent_idx_(0),
      is_request_tasks_full_(false),
      before_close_cb_(std::move(before_close_cb)),
      pending_response_count_(0),
      response_event_task_(task_processor_, [this] { SendResponses(); }),
      request_parser_(std::make_unique<http::HttpRequestParser>(
          request_handler.GetHandlerInfoIndex(), *config_.request,
          [this](std::unique_ptr<request::RequestBase>&& request_ptr) {
            NewRequest(std::move(request_ptr));
          })),
      is_closing_(false),
      processed_requests_count_(0),
      is_accepting_requests_(true),
      is_socket_listener_stopped_(false) {
  LOG_DEBUG() << "Incoming connection from " << peer_socket_.Getpeername()
              << ", fd " << Fd();
}

Connection::~Connection() {
  is_closing_ = true;

  const int fd = Fd();  // will be invalidated on close

  LOG_TRACE() << "Stopping socket listener for fd " << fd;
  socket_listener_ = {};
  LOG_TRACE() << "Stopped socket listener for fd " << fd;

  peer_socket_.Close();
  response_event_task_.Notify();
  LOG_TRACE() << "Waiting for request tasks queue to become empty for fd "
              << fd;
  {
    std::unique_lock<engine::Mutex> lock(request_tasks_mutex_);
    request_tasks_empty_cv_.Wait(lock,
                                 [this] { return request_tasks_.empty(); });
  }
  LOG_TRACE() << "RequestBase tasks queue became empty for fd " << fd;
  assert(IsRequestTasksEmpty());

  LOG_TRACE() << "Stopping response event notifier for fd " << fd;
  response_event_task_.Stop();
  LOG_TRACE() << "Stopped response event notifier for fd " << fd;

  LOG_DEBUG() << "Closed connection for fd " << fd;
}

void Connection::Start() {
  LOG_TRACE() << "Starting socket listener for fd " << Fd();
  // XXX: Connection is destroyed asynchronously, block the callback
  std::lock_guard<engine::Mutex> lock(close_cb_mutex_);
  socket_listener_ =
      engine::CriticalAsync(task_processor_, [this] { ListenForRequests(); });
  LOG_TRACE() << "Started socket listener for fd " << Fd();
}

int Connection::Fd() const { return peer_socket_.Fd(); }

Connection::Type Connection::GetType() const { return type_; }

size_t Connection::ProcessedRequestCount() const {
  return processed_requests_count_;
}

size_t Connection::ActiveRequestCount() const { return request_tasks_.size(); }

size_t Connection::ParsingRequestCount() const {
  return request_parser_->ParsingRequestCount();
}

size_t Connection::PendingResponseCount() const {
  return pending_response_count_;
}

bool Connection::CanClose() const {
  return is_socket_listener_stopped_ && IsRequestTasksEmpty();
}

bool Connection::IsRequestTasksEmpty() const {
  std::lock_guard<engine::Mutex> lock(request_tasks_mutex_);
  return request_tasks_.empty();
}

void Connection::ListenForRequests() {
  try {
    std::vector<char> buf(config_.in_buffer_size);
    while (is_accepting_requests_) {
      auto bytes_read = peer_socket_.RecvSome(buf.data(), buf.size(), {});
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

      std::unique_lock<engine::Mutex> lock(request_tasks_mutex_);
      request_tasks_full_cv_.Wait(lock, [this] {
        if (request_tasks_.size() >= config_.requests_queue_size_threshold) {
          LOG_TRACE() << "Receiving from fd " << Fd()
                      << " paused due to queue overfill";
          return false;
        }
        return true;
      });
    }
  } catch (const std::exception& ex) {
    LOG_ERROR() << "Error while receiving from peer "
                << peer_socket_.Getpeername() << " on fd " << Fd() << ": "
                << ex.what();
  }
  is_socket_listener_stopped_ = true;
  CloseIfFinished();
}

void Connection::NewRequest(
    std::unique_ptr<request::RequestBase>&& request_ptr) {
  if (request_ptr->IsFinal()) {
    is_accepting_requests_ = false;
  }
  auto request_task =
      request_handler_.PrepareRequestTask(std::move(request_ptr), [this] {
        ++pending_response_count_;
        response_event_task_.Notify();
      });

  {
    std::lock_guard<engine::Mutex> lock(request_tasks_mutex_);
    request_tasks_.push_back(request_task);
  }
  request_handler_.ProcessRequest(*request_task);
}

void Connection::SendResponses() {
  LOG_TRACE() << "Sending responses for fd " << Fd();
  for (;;) {
    request::RequestTask* task_ptr = nullptr;
    {
      std::lock_guard<engine::Mutex> lock(request_tasks_mutex_);
      if (request_tasks_sent_idx_ < request_tasks_.size() &&
          request_tasks_[request_tasks_sent_idx_]
              ->GetRequest()
              .GetResponse()
              .IsReady()) {
        task_ptr = request_tasks_[request_tasks_sent_idx_++].get();
      }
    }
    if (!task_ptr) break;

    auto& request = task_ptr->GetRequest();
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
    --pending_response_count_;
  }

  for (;;) {
    std::shared_ptr<request::RequestTask> request_task_ptr;
    {
      std::lock_guard<engine::Mutex> lock(request_tasks_mutex_);
      LOG_TRACE() << "Fd " << Fd() << " has " << request_tasks_.size()
                  << " request tasks";
      if (!request_tasks_.empty() && request_tasks_.front()->IsComplete() &&
          request_tasks_.front()->GetRequest().GetResponse().IsSent()) {
        assert(request_tasks_sent_idx_);
        --request_tasks_sent_idx_;
        request_task_ptr = std::move(request_tasks_.front());
        request_tasks_.pop_front();
        ++processed_requests_count_;

        if (request_tasks_.size() < config_.requests_queue_size_threshold) {
          request_tasks_full_cv_.NotifyOne();
        }
      }
    }
    if (request_task_ptr) {
      // We've received a notification from task, but it might not finished yet.
      request_task_ptr->WaitForTaskStop();

      request_task_ptr->GetRequest().WriteAccessLogs(
          request_handler_.LoggerAccess(), request_handler_.LoggerAccessTskv(),
          remote_address_);
      request_task_ptr.reset();
    } else {
      break;
    }
  }

  {
    std::lock_guard<engine::Mutex> lock(request_tasks_mutex_);
    if (request_tasks_.empty()) {
      request_tasks_empty_cv_.NotifyAll();
    }
  }

  CloseIfFinished();
}

void Connection::CloseIfFinished() {
  if (!is_closing_ && CanClose()) {
    is_closing_ = true;
  }
  if (is_closing_) {
    std::lock_guard<engine::Mutex> lock_cb(close_cb_mutex_);
    std::lock_guard<engine::Mutex> lock_tasks_queue(request_tasks_mutex_);
    if (request_tasks_.empty() && before_close_cb_) {
      before_close_cb_(Fd());
      before_close_cb_ = {};
    }
  }
}

}  // namespace net
}  // namespace server
