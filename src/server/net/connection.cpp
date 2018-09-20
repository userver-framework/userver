#include "connection.hpp"

#include <arpa/inet.h>
#include <netdb.h>
#include <sys/socket.h>
#include <unistd.h>

#include <algorithm>
#include <array>
#include <cassert>
#include <cerrno>
#include <stdexcept>
#include <system_error>
#include <vector>

#include <logging/log.hpp>
#include <server/http/http_request_handler.hpp>
#include <server/http/http_request_parser.hpp>
#include <server/request/request_config.hpp>

namespace server {
namespace net {

namespace {

// used for remote address buffer size
const size_t kMaxRemoteIdLength = INET6_ADDRSTRLEN;

}  // namespace

Connection::Connection(engine::ev::ThreadControl& thread_control,
                       engine::TaskProcessor& task_processor, int fd,
                       const ConnectionConfig& config, Type type,
                       const http::HttpRequestHandler& request_handler,
                       const sockaddr_in6& sin6, BeforeCloseCb before_close_cb)
    : task_processor_(task_processor),
      config_(config),
      type_(type),
      request_handler_(request_handler),
      request_tasks_sent_idx_(0),
      is_request_tasks_full_(false),
      before_close_cb_(std::move(before_close_cb)),
      request_parser_(std::make_unique<http::HttpRequestParser>(
          request_handler, *config_.request,
          [this](std::unique_ptr<request::RequestBase>&& request_ptr) {
            NewRequest(std::move(request_ptr));
          })),
      is_closing_(false),
      processed_requests_count_(0),
      socket_listener_stopped_{false} {
  response_event_task_ = std::make_shared<engine::EventTask>(
      task_processor_, [this] { SendResponses(); });
  response_sender_ = std::make_unique<engine::Sender>(
      thread_control, task_processor_, fd, [this] { CloseIfFinished(); });
  socket_listener_ = std::make_shared<engine::SocketListener>(
      thread_control, task_processor_, fd,
      engine::SocketListener::ListenMode::kRead,
      [this](int fd) { return ReadData(fd); },
      [this] {
        socket_listener_stop_time_ = std::chrono::steady_clock::now();
        socket_listener_stopped_ = true;  // Sequentially-consistent ordering is
                                          // important here for
                                          // socket_listener_stop_time_
        CloseIfFinished();
      });

  std::array<char, kMaxRemoteIdLength> buf;

  buf.fill('\0');
  auto* remote_address_cstr =
      inet_ntop(AF_INET6, &sin6.sin6_addr, buf.data(), buf.size());
  if (!remote_address_cstr) {
    throw std::system_error(errno, std::generic_category(),
                            "Cannot get remote address string");
  }
  remote_address_ = remote_address_cstr;
  remote_port_ = ntohs(sin6.sin6_port);

  LOG_DEBUG() << "Incoming connection from " << remote_address_ << ", port "
              << remote_port_ << ", fd " << Fd();
}

Connection::~Connection() {
  is_closing_ = true;
  LOG_TRACE() << "Stopping response sender for fd " << Fd();
  response_sender_->Stop();
  LOG_TRACE() << "Stopped response sender for fd " << Fd();

  response_event_task_->Notify();

  LOG_TRACE() << "Stopping socket listener for fd " << Fd();
  socket_listener_->Stop();
  LOG_TRACE() << "Stopped socket listener for fd " << Fd();

  close(Fd());
  LOG_TRACE() << "Waiting for request tasks queue to become empty for fd "
              << Fd();
  {
    std::unique_lock<engine::Mutex> lock(request_tasks_mutex_);
    request_tasks_empty_cv_.Wait(lock,
                                 [this] { return request_tasks_.empty(); });
  }
  LOG_TRACE() << "RequestBase tasks queue became empty for fd " << Fd();
  assert(IsRequestTasksEmpty());

  LOG_TRACE() << "Stopping response event notifier for fd " << Fd();
  response_event_task_->Stop();
  LOG_TRACE() << "Stopped response event notifier for fd " << Fd();

  LOG_DEBUG() << "Closed connection from " << remote_address_ << ", port "
              << remote_port_ << ", fd " << Fd();
}

void Connection::Start() {
  LOG_TRACE() << "Starting response sender for fd " << Fd();
  response_sender_->Start();
  LOG_TRACE() << "Started response sender for fd " << Fd();

  LOG_TRACE() << "Starting socket listener for fd " << Fd();
  socket_listener_->Start();
  LOG_TRACE() << "Started socket listener for fd " << Fd();
}

int Connection::Fd() const { return socket_listener_->Fd(); }

Connection::Type Connection::GetType() const { return type_; }

const std::string& Connection::RemoteAddress() const { return remote_address_; }

size_t Connection::ProcessedRequestCount() const {
  return processed_requests_count_;
}

size_t Connection::ActiveRequestCount() const { return request_tasks_.size(); }

size_t Connection::ParsingRequestCount() const {
  return request_parser_->ParsingRequestCount();
}

size_t Connection::PendingResponseCount() const {
  return response_sender_->DataQueueSize();
}

bool Connection::CanClose() const {
  return response_sender_->Stopped() ||
         (!socket_listener_->IsRunning() && IsRequestTasksEmpty() &&
          !response_sender_->HasWaitingData());
}

bool Connection::IsRequestTasksEmpty() const {
  std::lock_guard<engine::Mutex> lock(request_tasks_mutex_);
  return request_tasks_.empty();
}

engine::SocketListener::Result Connection::ReadData(int fd) {
  std::vector<char> buf(config_.in_buffer_size);
  int bytes_read = recv(fd, buf.data(), buf.size(), 0);
  LOG_TRACE() << "Received " << bytes_read << " byte(s) from fd " << Fd();
  if (bytes_read < 0) {
    if (errno == EAGAIN || errno == EWOULDBLOCK || errno == EINTR) {
      return engine::SocketListener::Result::kAgain;
    }
    std::error_code error(errno, std::generic_category());
    LOG_ERROR() << "Cannot receive data from fd " << Fd() << ": "
                << error.message();
    return engine::SocketListener::Result::kError;
  }
  if (bytes_read == 0 || !request_parser_->Parse(buf.data(), bytes_read) ||
      response_sender_->Stopped()) {
    return engine::SocketListener::Result::kError;
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
  return engine::SocketListener::Result::kOk;
}

void Connection::NewRequest(
    std::unique_ptr<request::RequestBase>&& request_ptr) {
  auto request_task = request_handler_.PrepareRequestTask(
      std::move(request_ptr),
      [task = response_event_task_] { task->Notify(); });

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
    response.SendResponse(
        *response_sender_,
        [this, &response, &request](size_t bytes_sent) {
          request.SetFinishSendResponseTime();
          response.SetSentTime(bytes_sent ? std::chrono::steady_clock::now()
                                          : socket_listener_stop_time_);
          response.SetSent(bytes_sent);
          response_event_task_->Notify();
        },
        !socket_listener_stopped_);
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
    std::lock_guard<engine::Mutex> lock(request_tasks_mutex_);
    if (request_tasks_.empty() && before_close_cb_) {
      before_close_cb_(Fd());
      before_close_cb_ = {};
    }
  }
}

}  // namespace net
}  // namespace server
