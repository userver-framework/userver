#pragma once

#include <atomic>
#include <memory>
#include <unordered_map>

#include <boost/lockfree/queue.hpp>

#include <engine/ev/thread_control.hpp>
#include <engine/event_task.hpp>
#include <engine/mutex.hpp>
#include <engine/socket_listener.hpp>
#include <engine/task/task_processor.hpp>

#include "connection.hpp"
#include "endpoint_info.hpp"
#include "stats.hpp"

namespace server {
namespace net {

int CreateIpv6Socket(uint16_t port, int backlog);

class ListenerImpl : public engine::ev::ThreadControl {
 public:
  ListenerImpl(engine::ev::ThreadControl& thread_control,
               engine::TaskProcessor& task_processor,
               std::shared_ptr<EndpointInfo> endpoint_info);
  ~ListenerImpl();

  Stats GetStats() const;

  engine::TaskProcessor& GetTaskProcessor() const;

 private:
  void AcceptConnection(int listen_fd, Connection::Type type);
  void SetupConnection(int fd, Connection::Type type, const sockaddr_in6& addr);

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
  std::atomic<size_t> pending_setup_connection_count_;
  std::atomic<size_t> pending_close_connection_count_;

  std::shared_ptr<engine::SocketListener> request_socket_listener_;
};

}  // namespace net
}  // namespace server
