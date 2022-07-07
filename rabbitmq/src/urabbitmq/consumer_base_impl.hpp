#pragma once

#include <engine/ev/thread_control.hpp>
#include <userver/concurrent/background_task_storage_fwd.hpp>
#include <userver/engine/task/task_processor_fwd.hpp>

#include <amqpcpp.h>

USERVER_NAMESPACE_BEGIN

namespace urabbitmq {

namespace impl {
class AmqpChannel;
}

class ConsumerBaseImpl final {
 public:
  ConsumerBaseImpl(impl::AmqpChannel& channel, const std::string& queue);
  ~ConsumerBaseImpl();

  using DispatchCallback = std::function<void(std::string message)>;

  void Start(DispatchCallback cb);
  void Stop();

 private:
  void OnMessage(const AMQP::Message& message, uint64_t delivery_tag);

  impl::AmqpChannel& channel_;
  engine::TaskProcessor& dispatcher_;
  const std::string queue_name_;

  std::optional<std::string> consumer_tag_;
  DispatchCallback dispatch_callback_;

  concurrent::BackgroundTaskStorageFastPimpl bts_;
  std::shared_ptr<bool> alive_;
};

}  // namespace urabbitmq

USERVER_NAMESPACE_END
