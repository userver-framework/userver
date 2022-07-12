#include <userver/urabbitmq/admin_channel.hpp>

#include <userver/urabbitmq/client.hpp>

#include <urabbitmq/channel_ptr.hpp>
#include <urabbitmq/impl/amqp_channel.hpp>

USERVER_NAMESPACE_BEGIN

namespace urabbitmq {

class AdminChannel::Impl final {
 public:
  Impl(ChannelPtr&& channel) : channel_{std::move(channel)} {}

  const ChannelPtr& Get() const { return channel_; }

 private:
  ChannelPtr channel_;
};

AdminChannel::AdminChannel(std::shared_ptr<Client>&& client,
                           ChannelPtr&& channel)
    : client_{std::move(client)}, impl_{std::move(channel)} {}

AdminChannel::~AdminChannel() = default;

AdminChannel::AdminChannel(AdminChannel&& other) noexcept = default;

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

void AdminChannel::RemoveExchange(const Exchange& exchange) {
  impl_->Get()->RemoveExchange(exchange);
}

void AdminChannel::RemoveQueue(const Queue& queue) {
  impl_->Get()->RemoveQueue(queue);
}

}  // namespace urabbitmq

USERVER_NAMESPACE_END