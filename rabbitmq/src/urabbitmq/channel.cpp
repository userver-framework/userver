#include <userver/urabbitmq/channel.hpp>

#include <urabbitmq/channel_ptr.hpp>
#include <urabbitmq/impl/amqp_channel.hpp>

USERVER_NAMESPACE_BEGIN

namespace urabbitmq {

Channel::Channel(ChannelPtr&& channel) : impl_{std::move(channel)} {}

Channel::~Channel() = default;

Channel::Channel(Channel&& other) noexcept = default;

void Channel::Publish(const Exchange& exchange, const std::string& routing_key,
                      const std::string& message, MessageType type) {
  // AmqpChannel ignores this deadline
  impl_->Get()->Publish(exchange, routing_key, message, type, {});
}

ReliableChannel::ReliableChannel(ChannelPtr&& channel)
    : impl_{std::move(channel)} {}

ReliableChannel::~ReliableChannel() = default;

ReliableChannel::ReliableChannel(ReliableChannel&& other) noexcept = default;

void ReliableChannel::Publish(const Exchange& exchange,
                              const std::string& routing_key,
                              const std::string& message, MessageType type,
                              engine::Deadline deadline) {
  impl_->Get()->Publish(exchange, routing_key, message, type, deadline);
}

}  // namespace urabbitmq

USERVER_NAMESPACE_END
