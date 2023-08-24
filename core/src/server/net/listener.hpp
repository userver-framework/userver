#pragma once

#include <memory>

#include <userver/engine/task/task_processor_fwd.hpp>

#include "endpoint_info.hpp"
#include "listener_impl.hpp"
#include "stats.hpp"

USERVER_NAMESPACE_BEGIN

namespace server::net {

class Listener final {
 public:
  Listener(std::shared_ptr<EndpointInfo> endpoint_info,
           engine::TaskProcessor& task_processor,
           request::ResponseDataAccounter& data_accounter);
  ~Listener();

  Listener(const Listener&) = delete;
  Listener(Listener&&) noexcept = default;
  Listener& operator=(const Listener&) = delete;
  Listener& operator=(Listener&&) = default;

  void Start();

  Stats GetStats() const;

 private:
  engine::TaskProcessor* task_processor_;
  std::shared_ptr<EndpointInfo> endpoint_info_;
  request::ResponseDataAccounter* data_accounter_;

  std::unique_ptr<ListenerImpl> impl_;
};

}  // namespace server::net

USERVER_NAMESPACE_END
