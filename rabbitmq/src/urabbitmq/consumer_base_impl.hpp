#pragma once

#include <engine/ev/thread_control.hpp>
#include <userver/concurrent/background_task_storage_fwd.hpp>
#include <userver/engine/task/task_processor_fwd.hpp>

#include <urabbitmq/connection_ptr.hpp>

#include <userver/urabbitmq/consumer_settings.hpp>

#include <amqpcpp.h>

USERVER_NAMESPACE_BEGIN

namespace urabbitmq {

namespace impl {
class AmqpChannel;
}

class ConsumerBaseImpl final {
 public:
  ConsumerBaseImpl(ConnectionPtr&& connection,
                   const ConsumerSettings& settings);
  ~ConsumerBaseImpl();

  using DispatchCallback = std::function<void(std::string message)>;

  void Start(DispatchCallback cb);
  void Stop();

  bool IsBroken() const;

 private:
  void OnMessage(const AMQP::Message& message, uint64_t delivery_tag);

  engine::TaskProcessor& dispatcher_;
  const std::string queue_name_;

  ConnectionPtr connection_ptr_;
  impl::AmqpChannel& channel_;

  // Not synchronized, only touch it from ev thread
  std::optional<std::string> consumer_tag_;

  DispatchCallback dispatch_callback_;

  bool started_{false};
  std::atomic<bool> stopped_{false};

  // Underlying channel errored, just restart the consumer
  // (consumer_base polls this and destructs+constructs us if we broke)
  std::atomic<bool> broken_{false};

  // This should be the last member
  concurrent::BackgroundTaskStorageFastPimpl bts_;
};

}  // namespace urabbitmq

USERVER_NAMESPACE_END
