#pragma once

#include <atomic>
#include <deque>
#include <functional>
#include <memory>
#include <string>

#include <engine/condition_variable.hpp>
#include <engine/mutex.hpp>
#include <server/request/request_base.hpp>

#include <engine/event_task.hpp>
#include <engine/io/socket.hpp>
#include <engine/task/task_processor.hpp>
#include <engine/task/task_with_result.hpp>
#include <server/http/http_request_handler.hpp>
#include <server/request/request_handler_base.hpp>
#include <server/request/request_parser.hpp>
#include <server/request/request_task.hpp>

#include "connection_config.hpp"

namespace server {
namespace net {

class Connection {
 public:
  using BeforeCloseCb = std::function<void(int)>;

  enum class Type { kRequest, kMonitor };

  Connection(engine::TaskProcessor& task_processor,
             const ConnectionConfig& config, engine::io::Socket peer_socket,
             Type type, const http::HttpRequestHandler& request_handler,
             BeforeCloseCb before_close_cb);
  ~Connection();

  void Start();

  int Fd() const;
  Type GetType() const;

  size_t ProcessedRequestCount() const;
  size_t ActiveRequestCount() const;
  size_t ParsingRequestCount() const;
  size_t PendingResponseCount() const;

 private:
  bool CanClose() const;
  bool IsRequestTasksEmpty() const;

  void ListenForRequests();

  void NewRequest(std::unique_ptr<request::RequestBase>&& request_ptr);
  void SendResponses();
  void CloseIfFinished();

  engine::TaskProcessor& task_processor_;
  const ConnectionConfig& config_;
  engine::io::Socket peer_socket_;
  Type type_;
  const request::RequestHandlerBase& request_handler_;
  const std::string remote_address_;

  mutable engine::Mutex request_tasks_mutex_;
  std::deque<std::shared_ptr<request::RequestTask>> request_tasks_;
  size_t request_tasks_sent_idx_;
  engine::ConditionVariable request_tasks_empty_cv_;
  bool is_request_tasks_full_;
  engine::ConditionVariable request_tasks_full_cv_;

  engine::Mutex close_cb_mutex_;
  BeforeCloseCb before_close_cb_;

  std::atomic<size_t> pending_response_count_;
  std::chrono::steady_clock::time_point send_failure_time_;
  engine::EventTask response_event_task_;
  std::unique_ptr<request::RequestParser> request_parser_;

  std::atomic<bool> is_closing_;

  size_t processed_requests_count_;

  std::atomic<bool> is_accepting_requests_;
  std::atomic<bool> is_socket_listener_stopped_;
  engine::TaskWithResult<void> socket_listener_;
};

}  // namespace net
}  // namespace server
