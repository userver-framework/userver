#include "listener.hpp"

#include <future>

#include <engine/async.hpp>
#include <logging/log.hpp>

namespace server {
namespace net {

Listener::Listener(std::shared_ptr<EndpointInfo> endpoint_info,
                   engine::TaskProcessor& task_processor,
                   engine::ev::ThreadControl& thread_control) {
  impl_ = std::make_unique<ListenerImpl>(thread_control, task_processor,
                                         std::move(endpoint_info));
}

Listener::~Listener() {
  if (!impl_) return;

  LOG_TRACE() << "Destroying listener";
  impl_.reset();
  LOG_TRACE() << "Destroyed listener";
}

Stats Listener::GetStats() const { return impl_->GetStats(); }

}  // namespace net
}  // namespace server
