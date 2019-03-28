#pragma once

#include <memory>

#include <engine/task/task_processor.hpp>

#include "endpoint_info.hpp"
#include "listener_impl.hpp"
#include "stats.hpp"

namespace server {
namespace net {

class Listener {
 public:
  Listener(std::shared_ptr<EndpointInfo> endpoint_info,
           engine::TaskProcessor& task_processor);
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

  std::unique_ptr<ListenerImpl> impl_;
};

}  // namespace net
}  // namespace server
