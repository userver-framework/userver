#pragma once

#include <netinet/in.h>

#include <atomic>
#include <deque>
#include <functional>
#include <memory>
#include <mutex>
#include <string>

#include <engine/condition_variable.hpp>
#include <engine/ev/thread_control.hpp>
#include <engine/event_task.hpp>
#include <engine/sender.hpp>
#include <engine/socket_listener.hpp>
#include <server/request/request_base.hpp>
#include <server/request/request_handler_base.hpp>
#include <server/request/request_parser.hpp>
#include <server/request/request_task.hpp>
#include <server/request_handlers/request_handlers.hpp>

#include "connection_config.hpp"

namespace server {
namespace net {

class Connection {
 public:
  using BeforeCloseCb = std::function<void(int)>;

  enum class Type { kRequest, kMonitor };

  Connection(engine::ev::ThreadControl& thread_control, int fd,
             const ConnectionConfig& config, Type type,
             const RequestHandlers& request_handlers, const sockaddr_in6& sin6,
             BeforeCloseCb before_close_cb);
  ~Connection();

  void Start();

  int Fd() const;
  Type GetType() const;

  const std::string& RemoteAddress() const;
  const std::string& RemoteHost() const;

  size_t ProcessedRequestCount() const;
  size_t ActiveRequestCount() const;
  size_t ParsingRequestCount() const;
  size_t PendingResponseCount() const;

 private:
  bool CanClose() const;
  bool IsRequestTasksEmpty() const;

  engine::SocketListener::Result ReadData(int fd);

  void NewRequest(std::unique_ptr<request::RequestBase>&& request_ptr);
  void SendResponses();
  void CloseIfFinished();

  const ConnectionConfig& config_;
  Type type_;
  const request::RequestHandlerBase& request_handler_;

  mutable std::mutex request_tasks_mutex_;
  std::deque<std::unique_ptr<request::RequestTask>> request_tasks_;
  size_t request_tasks_sent_idx_;
  engine::ConditionVariable request_tasks_empty_cv_;
  bool is_request_tasks_full_;
  engine::ConditionVariable request_tasks_full_cv_;

  BeforeCloseCb before_close_cb_;

  std::shared_ptr<engine::EventTask> response_event_task_;
  std::unique_ptr<request::RequestParser> request_parser_;

  std::atomic<bool> is_closing_;

  std::string remote_address_;
  std::string remote_host_;
  uint16_t remote_port_;

  size_t processed_requests_count_;

  std::unique_ptr<engine::Sender> response_sender_;
  std::unique_ptr<engine::SocketListener> socket_listener_;
};

}  // namespace net
}  // namespace server
