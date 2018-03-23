#include "listener.hpp"

#include <future>

#include <engine/async.hpp>
#include <logging/log.hpp>

namespace server {
namespace net {

Listener::Listener(const ListenerConfig& config,
                   engine::TaskProcessor& task_processor,
                   request_handling::RequestHandler& request_handler,
                   engine::ev::ThreadControl& thread_control) {
  std::packaged_task<decltype(impl_)()> task(
      [this, &config, &task_processor, &request_handler, &thread_control] {
        return std::make_unique<ListenerImpl>(thread_control, config,
                                              task_processor, request_handler);
      });
  auto future = task.get_future();
  engine::Async(task_processor, std::move(task));
  impl_ = future.get();
}

Listener::~Listener() {
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
