#include <userver/urabbitmq/admin_channel.hpp>

#include <userver/urabbitmq/cluster.hpp>

#include <urabbitmq/channel_ptr.hpp>
#include <urabbitmq/impl/amqp_channel.hpp>

USERVER_NAMESPACE_BEGIN

namespace urabbitmq {

class AdminChannel::Impl final {
 public:
  Impl(ChannelPtr&& channel) : channel_{std::move(channel)} {}

  ChannelPtr& Get() { return channel_; }

 private:
  ChannelPtr channel_;
};

AdminChannel::AdminChannel(std::shared_ptr<Cluster>&& cluster, ChannelPtr&& channel)
 : cluster_{std::move(cluster)}, impl_{std::make_unique<Impl>(std::move(channel))} {}

AdminChannel::~AdminChannel() = default;

void AdminChannel::DeclareExchange(const Exchange& exchange,
                                   ExchangeType type) {
  impl_->Get()->DeclareExchange(exchange, type);
}

void AdminChannel::DeclareQueue(const Queue& queue) {
  impl_->Get()->DeclareQueue(queue);
}

void AdminChannel::BindQueue(const Exchange& exchange, const Queue& queue,
                             const std::string& routing_key) {
  impl_->Get()->BindQueue(exchange, queue, routing_key);
}

}  // namespace urabbitmq

USERVER_NAMESPACE_END