#include <userver/urabbitmq/admin_channel.hpp>

#include <userver/tracing/span.hpp>

#include <urabbitmq/channel_ptr.hpp>
#include <urabbitmq/impl/amqp_channel.hpp>

USERVER_NAMESPACE_BEGIN

namespace urabbitmq {

AdminChannel::AdminChannel(ChannelPtr&& channel) : impl_{std::move(channel)} {}

AdminChannel::~AdminChannel() = default;

AdminChannel::AdminChannel(AdminChannel&& other) noexcept = default;

void AdminChannel::DeclareExchange(const Exchange& exchange,
                                   Exchange::Type type,
                                   utils::Flags<Exchange::Flags> flags,
                                   engine::Deadline deadline) {
  tracing::Span span{"declare_exchange"};
  impl_->Get()->DeclareExchange(exchange, type, flags, deadline);
}

void AdminChannel::DeclareQueue(const Queue& queue,
                                utils::Flags<Queue::Flags> flags,
                                engine::Deadline deadline) {
  tracing::Span span{"declare_queue"};
  impl_->Get()->DeclareQueue(queue, flags, deadline);
}

void AdminChannel::BindQueue(const Exchange& exchange, const Queue& queue,
                             const std::string& routing_key,
                             engine::Deadline deadline) {
  tracing::Span span{"bind_queue"};
  impl_->Get()->BindQueue(exchange, queue, routing_key, deadline);
}

void AdminChannel::RemoveExchange(const Exchange& exchange,
                                  engine::Deadline deadline) {
  tracing::Span span{"remove_exchange"};
  impl_->Get()->RemoveExchange(exchange, deadline);
}

void AdminChannel::RemoveQueue(const Queue& queue, engine::Deadline deadline) {
  tracing::Span span{"remove_queue"};
  impl_->Get()->RemoveQueue(queue, deadline);
}

}  // namespace urabbitmq

USERVER_NAMESPACE_END