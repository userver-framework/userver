#pragma once

#include <atomic>
#include <memory>
#include <unordered_map>

#include <boost/lockfree/queue.hpp>

#include <engine/event_task.hpp>
#include <engine/io/socket.hpp>
#include <engine/mutex.hpp>
#include <engine/task/task_processor.hpp>
#include <engine/task/task_with_result.hpp>

#include "connection.hpp"
#include "endpoint_info.hpp"
#include "stats.hpp"

namespace server {
namespace net {

class ListenerImpl {
 public:
  ListenerImpl(engine::TaskProcessor& task_processor,
               std::shared_ptr<EndpointInfo> endpoint_info);
  ~ListenerImpl();

  Stats GetStats() const;

  engine::TaskProcessor& GetTaskProcessor() const;

 private:
  void AcceptConnection(engine::io::Socket& request_socket, Connection::Type);
  void SetupConnection(engine::io::Socket peer_socket, Connection::Type);

  void EnqueueConnectionClose(int fd);
  void CloseConnections();

  engine::TaskProcessor& task_processor_;
  std::shared_ptr<EndpointInfo> endpoint_info_;

  bool is_closing_;

  mutable engine::Mutex connections_mutex_;
  std::unordered_map<int, std::unique_ptr<Connection>> connections_;

  boost::lockfree::queue<int> connections_to_close_;
  engine::EventTask close_connections_task_;
  std::atomic<size_t> past_processed_requests_count_;
  std::atomic<size_t> opened_connection_count_;
  std::atomic<size_t> closed_connection_count_;
  std::atomic<size_t> pending_close_connection_count_;

  engine::TaskWithResult<void> socket_listener_task_;
};

}  // namespace net
}  // namespace server
