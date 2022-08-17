#include <userver/urabbitmq/channel.hpp>

#include <userver/tracing/span.hpp>

#include <urabbitmq/connection.hpp>
#include <urabbitmq/connection_ptr.hpp>
#include <urabbitmq/impl/amqp_channel.hpp>

USERVER_NAMESPACE_BEGIN

namespace urabbitmq {

Channel::Channel(ConnectionPtr&& channel) : impl_{std::move(channel)} {}

Channel::~Channel() = default;

Channel::Channel(Channel&& other) noexcept = default;

void Channel::Publish(const Exchange& exchange, const std::string& routing_key,
                      const std::string& message, MessageType type,
                      engine::Deadline deadline) {
  tracing::Span span{"publish"};
  (*impl_)->GetChannel().Publish(exchange, routing_key, message, type,
                                 deadline);
}

ReliableChannel::ReliableChannel(ConnectionPtr&& channel)
    : impl_{std::move(channel)} {}

ReliableChannel::~ReliableChannel() = default;

ReliableChannel::ReliableChannel(ReliableChannel&& other) noexcept = default;

void ReliableChannel::Publish(const Exchange& exchange,
                              const std::string& routing_key,
                              const std::string& message, MessageType type,
                              engine::Deadline deadline) {
  tracing::Span span{"reliable_publish"};
  auto awaiter = (*impl_)->GetReliableChannel().Publish(
      exchange, routing_key, message, type, deadline);
  awaiter.Wait(deadline);
}

}  // namespace urabbitmq

USERVER_NAMESPACE_END
