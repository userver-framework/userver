#include <userver/urabbitmq/admin_channel.hpp>

#include <userver/tracing/span.hpp>

#include <urabbitmq/connection.hpp>
#include <urabbitmq/connection_ptr.hpp>
#include <urabbitmq/impl/amqp_channel.hpp>

USERVER_NAMESPACE_BEGIN

namespace urabbitmq {

AdminChannel::AdminChannel(ConnectionPtr&& channel)
    : impl_{std::move(channel)} {}

AdminChannel::~AdminChannel() = default;

AdminChannel::AdminChannel(AdminChannel&& other) noexcept = default;

void AdminChannel::DeclareExchange(const Exchange& exchange,
                                   Exchange::Type type,
                                   utils::Flags<Exchange::Flags> flags,
                                   engine::Deadline deadline) {
  tracing::Span span{"declare_exchange"};
  auto awaiter =
      (*impl_)->GetChannel().DeclareExchange(exchange, type, flags, deadline);
  awaiter.Wait(deadline);
}

void AdminChannel::DeclareQueue(const Queue& queue,
                                utils::Flags<Queue::Flags> flags,
                                engine::Deadline deadline) {
  tracing::Span span{"declare_queue"};
  auto awaiter = (*impl_)->GetChannel().DeclareQueue(queue, flags, deadline);
  awaiter.Wait(deadline);
}

void AdminChannel::BindQueue(const Exchange& exchange, const Queue& queue,
                             const std::string& routing_key,
                             engine::Deadline deadline) {
  tracing::Span span{"bind_queue"};
  auto awaiter =
      (*impl_)->GetChannel().BindQueue(exchange, queue, routing_key, deadline);
  awaiter.Wait(deadline);
}

void AdminChannel::RemoveExchange(const Exchange& exchange,
                                  engine::Deadline deadline) {
  tracing::Span span{"remove_exchange"};
  auto awaiter = (*impl_)->GetChannel().RemoveExchange(exchange, deadline);
  awaiter.Wait(deadline);
}

void AdminChannel::RemoveQueue(const Queue& queue, engine::Deadline deadline) {
  tracing::Span span{"remove_queue"};
  auto awaiter = (*impl_)->GetChannel().RemoveQueue(queue, deadline);
  awaiter.Wait(deadline);
}

}  // namespace urabbitmq

USERVER_NAMESPACE_END