#pragma once

#include <memory>

#include <userver/concurrent/background_task_storage.hpp>
#include <userver/engine/io/socket.hpp>
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

 private:
  void AcceptConnection(engine::io::Socket& request_socket);
  void ProcessConnection(engine::io::Socket peer_socket);

  engine::TaskProcessor& task_processor_;
  std::shared_ptr<EndpointInfo> endpoint_info_;

  std::shared_ptr<Stats> stats_;
  request::ResponseDataAccounter& data_accounter_;

  concurrent::BackgroundTaskStorageCore connections_;

  engine::TaskWithResult<void> socket_listener_task_;
};

}  // namespace server::net

USERVER_NAMESPACE_END
