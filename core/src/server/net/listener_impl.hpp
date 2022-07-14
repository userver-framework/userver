#pragma once

#include <atomic>
#include <memory>
#include <unordered_map>

#include <userver/engine/io/socket.hpp>
#include <userver/engine/mutex.hpp>
#include <userver/engine/task/task_processor_fwd.hpp>
#include <userver/engine/task/task_with_result.hpp>

#include "connection.hpp"
#include "endpoint_info.hpp"
#include "stats.hpp"

USERVER_NAMESPACE_BEGIN

namespace server::net {

class ListenerImpl final {
 public:
  ListenerImpl(engine::TaskProcessor& task_processor,
               std::shared_ptr<EndpointInfo> endpoint_info,
               request::ResponseDataAccounter& data_accounter);
  ~ListenerImpl();

  Stats GetStats() const;

  engine::TaskProcessor& GetTaskProcessor() const;

 private:
  void AcceptConnection(engine::io::Socket& request_socket);

  void SetupConnection(engine::io::Socket peer_socket);

  void AddConnection(const std::shared_ptr<Connection>&);

  void CloseConnections();

  engine::TaskProcessor& task_processor_;
  std::shared_ptr<EndpointInfo> endpoint_info_;

  std::shared_ptr<Stats> stats_;
  request::ResponseDataAccounter& data_accounter_;

  engine::TaskWithResult<void> socket_listener_task_;

  // connections_ are added in socket_listener_task_ and removed
  // in ~ListenerImpl(), no synchronization required
  std::vector<std::weak_ptr<Connection>> connections_;
};

}  // namespace server::net

USERVER_NAMESPACE_END
