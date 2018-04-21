#include "listener.hpp"

#include <future>

#include <engine/async.hpp>
#include <logging/log.hpp>

namespace server {
namespace net {

Listener::Listener(std::shared_ptr<EndpointInfo> endpoint_info,
                   engine::TaskProcessor& task_processor,
                   engine::ev::ThreadControl& thread_control) {
  std::packaged_task<decltype(impl_)()> task([
    endpoint_info = std::move(endpoint_info), &task_processor, &thread_control
  ] {
    return std::make_unique<ListenerImpl>(thread_control, task_processor,
                                          std::move(endpoint_info));
  });
  auto future = task.get_future();
  engine::Async(task_processor, std::move(task));
  impl_ = future.get();
}

Listener::~Listener() {
  if (!impl_) return;

  std::packaged_task<void()> task([this] {
    LOG_TRACE() << "Destroying listener";
    impl_.reset();
    LOG_TRACE() << "Destroyed listener";
  });
  auto future = task.get_future();
  engine::Async(impl_->GetTaskProcessor(), std::move(task));
  future.get();
}

Stats Listener::GetStats() const { return impl_->GetStats(); }

}  // namespace net
}  // namespace server
