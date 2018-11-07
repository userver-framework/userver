#pragma once

#include <atomic>
#include <deque>
#include <functional>
#include <memory>
#include <string>

#include <moodycamel/concurrentqueue.h>

#include <engine/condition_variable.hpp>
#include <engine/mutex.hpp>
#include <server/request/request_base.hpp>

#include <engine/io/socket.hpp>
#include <engine/single_consumer_event.hpp>
#include <engine/task/task_processor.hpp>
#include <engine/task/task_with_result.hpp>
#include <server/http/http_request_handler.hpp>
#include <server/net/stats.hpp>
#include <server/request/request_handler_base.hpp>
#include <server/request/request_parser.hpp>
#include <utils/work_control.hpp>

#include "connection_config.hpp"

namespace server {
namespace net {

class Connection : public std::enable_shared_from_this<Connection> {
 public:
  using CloseCb = std::function<void()>;

  enum class Type { kRequest, kMonitor };

  Connection(engine::TaskProcessor& task_processor,
             const ConnectionConfig& config, engine::io::Socket peer_socket,
             const http::HttpRequestHandler& request_handler,
             std::shared_ptr<Stats> stats);
  ~Connection();

  void SetCloseCb(CloseCb close_cb);

  void Start();

  void Stop();  // Can be called after Start() has finished

  int Fd() const;

 private:
  bool IsRequestTasksEmpty() const;

  void ListenForRequests();

  void NewRequest(std::shared_ptr<request::RequestBase>&& request_ptr);

  using QueueItem = std::pair<std::shared_ptr<request::RequestBase>,
                              engine::TaskWithResult<void>>;

  QueueItem DequeueRequestTask();

  void HandleQueueItem(QueueItem& item);

  void SendResponses();

  void StopSocketListenersAsync();
  void StopResponseSenderTaskAsync();
  void CloseAsync();

 private:
  engine::TaskProcessor& task_processor_;
  const ConnectionConfig& config_;
  engine::io::Socket peer_socket_;
  Type type_;
  const request::RequestHandlerBase& request_handler_;
  const std::shared_ptr<Stats> stats_;
  const std::string remote_address_;

  moodycamel::ConcurrentQueue<QueueItem> request_tasks_;
  engine::SingleConsumerEvent request_tasks_empty_event_,
      request_task_full_event_;

  std::chrono::steady_clock::time_point send_failure_time_;
  engine::TaskWithResult<void> response_sender_task_;
  std::unique_ptr<request::RequestParser> request_parser_;

  std::atomic<bool> is_accepting_requests_;
  std::atomic<bool> stop_sender_after_queue_is_empty_;
  std::atomic<bool> is_closing_;
  engine::TaskWithResult<void> socket_listener_;
  CloseCb close_cb_;

  std::shared_ptr<Connection> shared_this_;
};

}  // namespace net
}  // namespace server
