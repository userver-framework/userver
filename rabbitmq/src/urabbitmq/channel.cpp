#include <userver/urabbitmq/channel.hpp>

#include <urabbitmq/channel_ptr.hpp>
#include <urabbitmq/impl/amqp_channel.hpp>

USERVER_NAMESPACE_BEGIN

namespace urabbitmq {

class Channel::Impl final {
 public:
  Impl(ChannelPtr&& unreliable, ChannelPtr&& reliable)
      : unreliable_{std::move(unreliable)}, reliable_{std::move(reliable)} {}

  ChannelPtr& Get() { return unreliable_; }
  ChannelPtr& GetReliable() { return reliable_; }

 private:
  ChannelPtr unreliable_;
  ChannelPtr reliable_;
};

Channel::Channel(std::shared_ptr<Cluster>&& cluster, ChannelPtr&& channel,
                 ChannelPtr&& reliable)
    : cluster_{std::move(cluster)},
      impl_{std::make_unique<Impl>(std::move(channel), std::move(reliable))} {}

Channel::~Channel() = default;

void Channel::Publish(const Exchange& exchange, const std::string& routing_key,
                      const std::string& message) {
  impl_->Get()->Publish(exchange, routing_key, message);
}

void Channel::PublishReliable(const Exchange& exchange,
                              const std::string& routing_key,
                              const std::string& message) {
  impl_->GetReliable()->Publish(exchange, routing_key, message);
}

}  // namespace urabbitmq

USERVER_NAMESPACE_END
