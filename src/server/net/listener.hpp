#pragma once

#include <memory>

#include <engine/ev/thread_control.hpp>
#include <engine/task/task_processor.hpp>
#include <server/request_handling/request_handler.hpp>

#include "endpoint_info.hpp"
#include "listener_impl.hpp"
#include "stats.hpp"

namespace server {
namespace net {

class Listener {
 public:
  Listener(std::shared_ptr<EndpointInfo> endpoint_info,
           engine::TaskProcessor& task_processor,
           engine::ev::ThreadControl& thread_control);
  ~Listener();

  Listener(const Listener&) = delete;
  Listener(Listener&&) noexcept = default;
  Listener& operator=(const Listener&) = delete;
  Listener& operator=(Listener&&) = default;

  Stats GetStats() const;

 private:
  std::unique_ptr<ListenerImpl> impl_;
};

}  // namespace net
}  // namespace server
