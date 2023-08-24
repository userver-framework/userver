#include <userver/urabbitmq/channel.hpp>

#include <urabbitmq/connection_helper.hpp>
#include <urabbitmq/connection_ptr.hpp>

USERVER_NAMESPACE_BEGIN

namespace urabbitmq {

Channel::Channel(ConnectionPtr&& channel) : impl_{std::move(channel)} {}

Channel::~Channel() = default;

Channel::Channel(Channel&& other) noexcept = default;

void Channel::Publish(const Exchange& exchange, const std::string& routing_key,
                      const std::string& message, MessageType type,
                      engine::Deadline deadline) {
  ConnectionHelper::Publish(*impl_, exchange, routing_key, message, type,
                            deadline);
}

ReliableChannel::ReliableChannel(ConnectionPtr&& channel)
    : impl_{std::move(channel)} {}

ReliableChannel::~ReliableChannel() = default;

ReliableChannel::ReliableChannel(ReliableChannel&& other) noexcept = default;

void ReliableChannel::PublishReliable(const Exchange& exchange,
                                      const std::string& routing_key,
                                      const std::string& message,
                                      MessageType type,
                                      engine::Deadline deadline) {
  ConnectionHelper::PublishReliable(*impl_, exchange, routing_key, message,
                                    type, deadline)
      .Wait(deadline);
}

}  // namespace urabbitmq

USERVER_NAMESPACE_END
