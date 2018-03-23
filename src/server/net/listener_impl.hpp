#pragma once

#include <atomic>
#include <memory>
#include <mutex>
#include <unordered_map>

#include <boost/lockfree/queue.hpp>

#include <engine/ev/thread_control.hpp>
#include <engine/event_task.hpp>
#include <engine/socket_listener.hpp>
#include <engine/task/task_processor.hpp>
#include <server/request_handling/request_handler.hpp>

#include "connection.hpp"
#include "listener_config.hpp"
#include "stats.hpp"

namespace server {
namespace net {

class ListenerImpl : public engine::ev::ThreadControl {
 public:
  ListenerImpl(engine::ev::ThreadControl& thread_control,
               const ListenerConfig& config,
               engine::TaskProcessor& task_processor,
               request_handling::RequestHandler& request_handler);
  ~ListenerImpl();

  Stats GetStats() const;

  engine::TaskProcessor& GetTaskProcessor() const;

 private:
  void AcceptConnection(int listen_fd, Connection::Type type);

  void EnqueueConnectionClose(int fd);
  void CloseConnections();

  const ListenerConfig& config_;
  engine::TaskProcessor& task_processor_;
  request_handling::RequestHandler& request_handler_;

  bool is_closing_;

  mutable std::mutex connections_mutex_;
  std::unordered_map<int, std::unique_ptr<Connection>> connections_;
  boost::lockfree::queue<int> connections_to_close_;
  engine::EventTask close_connections_task_;
  std::atomic<size_t> past_processed_requests_count_;

  std::unique_ptr<engine::SocketListener> request_socket_listener_;
  std::unique_ptr<engine::SocketListener> monitor_socket_listener_;
};

}  // namespace net
}  // namespace server
