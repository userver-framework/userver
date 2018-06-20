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

#include <engine/task/task_processor.hpp>
#include <logging/log.hpp>
#include <server/http/http_request_handler.hpp>
#include <server/http/http_request_parser.hpp>
#include <server/request/request_config.hpp>

namespace server {
namespace net {

namespace {

// used for remote address/hostname buffer size
const size_t kMaxRemoteIdLength = std::max(INET6_ADDRSTRLEN, NI_MAXHOST);

std::unique_ptr<request::RequestParser> GetRequestParser(
    const request::RequestConfig& config, Connection::Type type,
    std::function<void(std::unique_ptr<request::RequestBase>&&)> on_new_request,
    const RequestHandlers& request_handlers) {
  if (type == Connection::Type::kMonitor ||
      config.GetType() == request::RequestConfig::Type::kHttp) {
    auto request_parser = std::make_unique<http::HttpRequestParser>(
        (type == Connection::Type::kMonitor
             ? request_handlers.GetMonitorRequestHandler()
             : request_handlers.GetHttpRequestHandler()),
        config, std::move(on_new_request));
    return request_parser;
  }
  throw std::runtime_error(
      "unknown request type: " +
      request::RequestConfig::TypeToString(config.GetType()));
}

const request::RequestHandlerBase& GetRequestHandler(
    const request::RequestConfig& config, Connection::Type type,
    const RequestHandlers& request_handlers) {
  if (type == Connection::Type::kMonitor)
    return request_handlers.GetMonitorRequestHandler();
  if (config.GetType() == request::RequestConfig::Type::kHttp)
    return request_handlers.GetHttpRequestHandler();
  throw std::runtime_error(
      "unknown request type: " +
      request::RequestConfig::TypeToString(config.GetType()));
}

}  // namespace

Connection::Connection(engine::ev::ThreadControl& thread_control, int fd,
                       const ConnectionConfig& config, Type type,
                       const RequestHandlers& request_handlers,
                       const sockaddr_in6& sin6, BeforeCloseCb before_close_cb)
    : config_(config),
      type_(type),
      request_handler_(
          GetRequestHandler(*config_.request, type, request_handlers)),
      request_tasks_sent_idx_(0),
      is_request_tasks_full_(false),
      before_close_cb_(std::move(before_close_cb)),
      request_parser_(GetRequestParser(
          *config_.request, type,
          [this](std::unique_ptr<request::RequestBase>&& request_ptr) {
            NewRequest(std::move(request_ptr));
          },
          request_handlers)),
      is_closing_(false),
      processed_requests_count_(0) {
  auto* task_processor_ptr =
      request_handler_.GetComponentContext().GetTaskProcessor(
          config_.task_processor);
  if (!task_processor_ptr) {
    throw std::runtime_error("Cannot find task processor '" +
                             config_.task_processor + '\'');
  }

  response_event_task_ = std::make_shared<engine::EventTask>(
      *task_processor_ptr, [this] { SendResponses(); });
  response_sender_ = std::make_unique<engine::Sender>(
      thread_control, *task_processor_ptr, fd, [this] { CloseIfFinished(); });
  socket_listener_ = std::make_unique<engine::SocketListener>(
      thread_control, *task_processor_ptr, fd,
      engine::SocketListener::ListenMode::kRead,
      [this](int fd) { return ReadData(fd); }, [this] { CloseIfFinished(); },
      engine::SocketListener::DeferStart{});

  std::array<char, kMaxRemoteIdLength> buf;

  buf.fill('\0');
  auto* remote_address_cstr =
      inet_ntop(AF_INET6, &sin6.sin6_addr, buf.data(), buf.size());
  if (!remote_address_cstr) {
    throw std::system_error(errno, std::generic_category(),
                            "Cannot get remote address string");
  }
  remote_address_ = remote_address_cstr;

  buf.fill('\0');
  auto gai_error =
      getnameinfo(reinterpret_cast<const sockaddr*>(&sin6), sizeof(sin6),
                  buf.data(), buf.size(), nullptr, 0, 0);
  if (gai_error) {
    if (gai_error == EAI_NONAME) {
      remote_host_ = remote_address_;
    } else {
      LOG_WARNING() << "Cannot get remote hostname: "
                    << gai_strerror(gai_error);
    }
  } else {
    remote_host_ = buf.data();
  }

  LOG_DEBUG() << "Incoming connection from " << remote_host_ << " ("
              << remote_address_ << ')';
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
    std::unique_lock<std::mutex> lock(request_tasks_mutex_);
    request_tasks_empty_cv_.Wait(lock,
                                 [this] { return request_tasks_.empty(); });
  }
  LOG_TRACE() << "RequestBase tasks queue became empty for fd " << Fd();
  assert(IsRequestTasksEmpty());

  LOG_TRACE() << "Stopping response event notifier for fd " << Fd();
  response_event_task_->Stop();
  LOG_TRACE() << "Stopped response event notifier for fd " << Fd();

  LOG_DEBUG() << "Closed connection from " << remote_host_ << " ("
              << remote_address_ << "), fd " << Fd();
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

const std::string& Connection::RemoteHost() const { return remote_host_; }

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
  std::lock_guard<std::mutex> lock(request_tasks_mutex_);
  return request_tasks_.empty();
}

engine::SocketListener::Result Connection::ReadData(int fd) {
  std::vector<char> buf(config_.in_buffer_size);
  int bytes_read = recv(fd, buf.data(), buf.size(), 0);
  LOG_TRACE() << "Received " << bytes_read << " from fd " << Fd();
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

  std::unique_lock<std::mutex> lock(request_tasks_mutex_);
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

  auto* task_ptr = request_task.get();
  assert(task_ptr);
  {
    std::lock_guard<std::mutex> lock(request_tasks_mutex_);
    request_tasks_.push_back(std::move(request_task));
  }
  request_handler_.ProcessRequest(*task_ptr);
}

void Connection::SendResponses() {
  LOG_TRACE() << "Sending responses for fd " << Fd();
  for (;;) {
    request::RequestTask* task_ptr = nullptr;
    {
      std::lock_guard<std::mutex> lock(request_tasks_mutex_);
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
    response.SendResponse(*response_sender_,
                          [this, &response, &request](size_t bytes_sent) {
                            request.SetFinishSendResponseTime();
                            response.SetSent(bytes_sent);
                            response_event_task_->Notify();
                          });
  }

  for (;;) {
    std::unique_ptr<request::RequestTask> request_task_ptr;
    {
      std::lock_guard<std::mutex> lock(request_tasks_mutex_);
      LOG_TRACE() << "Fd " << Fd() << " has " << request_tasks_.size()
                  << " request tasks";
      if (!request_tasks_.empty() && request_tasks_.front()->Finished() &&
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
          remote_host_, remote_address_);
      request_task_ptr.reset();
    } else {
      break;
    }
  }

  {
    std::lock_guard<std::mutex> lock(request_tasks_mutex_);
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
    std::lock_guard<std::mutex> lock(request_tasks_mutex_);
    if (request_tasks_.empty() && before_close_cb_) {
      before_close_cb_(Fd());
      before_close_cb_ = {};
    }
  }
}

}  // namespace net
}  // namespace server
