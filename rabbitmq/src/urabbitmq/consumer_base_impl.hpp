#pragma once

#include <engine/ev/thread_control.hpp>
#include <userver/concurrent/background_task_storage_fwd.hpp>
#include <userver/engine/task/task_processor_fwd.hpp>3

#include <urabbitmq/channel_ptr.hpp>

#include <amqpcpp.h>

USERVER_NAMESPACE_BEGIN

namespace urabbitmq {

namespace impl {
class AmqpChannel;
}

class ConsumerBaseImpl final {
 public:
  ConsumerBaseImpl(ChannelPtr&& channel, const std::string& queue);
  ~ConsumerBaseImpl();

  using DispatchCallback = std::function<void(std::string message)>;

  void Start(DispatchCallback cb);
  void Stop();

 private:
  void OnMessage(const AMQP::Message& message, uint64_t delivery_tag);

  ChannelPtr channel_ptr_;
  impl::AmqpChannel* channel_{};
  engine::TaskProcessor& dispatcher_;
  const std::string queue_name_;

  // Not synchronized, only touch it from ev thread
  std::optional<std::string> consumer_tag_;
  DispatchCallback dispatch_callback_;

  concurrent::BackgroundTaskStorageFastPimpl bts_;

  // In the underlying library a consumer is owned by the channel on which it
  // was created/started, thus a channel might outlive a consumer (and maybe the
  // architecture should be changed for that to not happen). Since consumer
  // operations are async by nature, there might be a situation when we don't
  // actually know if a consumer was started when we destroy it, and we need
  // this ugly hackery to ensure onSuccess/onMessage callbacks don`t fire on a
  // destroyed consumer. Keep in mind that this isn't synchronized and should
  // only be touched from ev thread
  std::shared_ptr<bool> alive_;
  bool stopped_{false};
};

}  // namespace urabbitmq

USERVER_NAMESPACE_END
