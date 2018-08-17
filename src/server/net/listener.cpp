#include "listener.hpp"

#include <engine/async.hpp>
#include <logging/log.hpp>

namespace server {
namespace net {

Listener::Listener(std::shared_ptr<EndpointInfo> endpoint_info,
                   engine::TaskProcessor& task_processor,
                   engine::ev::ThreadControl& thread_control)
    : thread_control_(thread_control),
      task_processor_(task_processor),
      endpoint_info_(std::move(endpoint_info)) {}

Listener::~Listener() {
  if (!impl_) return;

  LOG_TRACE() << "Destroying listener";
  impl_.reset();
  LOG_TRACE() << "Destroyed listener";
}

void Listener::Start() {
  impl_ = std::make_unique<ListenerImpl>(thread_control_, task_processor_,
                                         endpoint_info_);
}

Stats Listener::GetStats() const {
  if (impl_) return impl_->GetStats();
  return Stats{};
}

}  // namespace net
}  // namespace server
