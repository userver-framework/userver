#include "connection_helper.hpp"

#include <urabbitmq/connection.hpp>
#include <urabbitmq/connection_ptr.hpp>

USERVER_NAMESPACE_BEGIN

namespace urabbitmq {

impl::ResponseAwaiter ConnectionHelper::DeclareExchange(
    const ConnectionPtr& connection, const Exchange& exchange,
    Exchange::Type type, utils::Flags<Exchange::Flags> flags,
    engine::Deadline deadline) {
  return WithSpan("declare_exchange", [&] {
    return connection->GetChannel().DeclareExchange(exchange, type, flags,
                                                    deadline);
  });
}

impl::ResponseAwaiter ConnectionHelper::DeclareQueue(
    const ConnectionPtr& connection, const Queue& queue,
    utils::Flags<Queue::Flags> flags, engine::Deadline deadline) {
  return WithSpan("declare_queue", [&] {
    return connection->GetChannel().DeclareQueue(queue, flags, deadline);
  });
}

impl::ResponseAwaiter ConnectionHelper::BindQueue(
    const ConnectionPtr& connection, const Exchange& exchange,
    const Queue& queue, const std::string& routing_key,
    engine::Deadline deadline) {
  return WithSpan("bind_queue", [&] {
    return connection->GetChannel().BindQueue(exchange, queue, routing_key,
                                              deadline);
  });
}

impl::ResponseAwaiter ConnectionHelper::RemoveExchange(
    const ConnectionPtr& connection, const Exchange& exchange,
    engine::Deadline deadline) {
  return WithSpan("remove_exchange", [&] {
    return connection->GetChannel().RemoveExchange(exchange, deadline);
  });
}

impl::ResponseAwaiter ConnectionHelper::RemoveQueue(
    const ConnectionPtr& connection, const Queue& queue,
    engine::Deadline deadline) {
  return WithSpan("remove_queue", [&] {
    return connection->GetChannel().RemoveQueue(queue, deadline);
  });
}

void ConnectionHelper::Publish(const ConnectionPtr& connection,
                               const Exchange& exchange,
                               const std::string& routing_key,
                               const std::string& message, MessageType type,
                               engine::Deadline deadline) {
  tracing::Span span{"publish"};
  connection->GetChannel().Publish(exchange, routing_key, message, type,
                                   deadline);
}

impl::ResponseAwaiter ConnectionHelper::PublishReliable(
    const ConnectionPtr& connection, const Exchange& exchange,
    const std::string& routing_key, const std::string& message,
    MessageType type, engine::Deadline deadline) {
  return WithSpan("reliable_publish", [&] {
    return connection->GetReliableChannel().Publish(exchange, routing_key,
                                                    message, type, deadline);
  });
}

}  // namespace urabbitmq

USERVER_NAMESPACE_END
