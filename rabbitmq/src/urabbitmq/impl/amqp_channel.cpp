#include "amqp_channel.hpp"

#include <optional>

#include <userver/engine/task/task.hpp>
#include <userver/tracing/span.hpp>

#include <urabbitmq/impl/amqp_connection.hpp>
#include <urabbitmq/impl/deferred_wrapper.hpp>
#include <urabbitmq/statistics/connection_statistics.hpp>

USERVER_NAMESPACE_BEGIN

namespace urabbitmq::impl {

namespace {

AMQP::ExchangeType Convert(urabbitmq::Exchange::Type type) {
  using From = urabbitmq::Exchange::Type;
  using To = AMQP::ExchangeType;

  switch (type) {
    case From::kFanOut:
      return To::fanout;
    case From::kDirect:
      return To::direct;
    case From::kTopic:
      return To::topic;
    case From::kHeaders:
      return To::headers;
    case From::kConsistentHash:
      return To::consistent_hash;
    case From::kMessageDeduplication:
      return To::message_deduplication;
  }

  UINVARIANT(false, "Should be unreachable, fix the switch");
}

int Convert(utils::Flags<Queue::Flags> flags) {
  int result = 0;
  if (flags & Queue::Flags::kPassive) result |= AMQP::passive;
  if (flags & Queue::Flags::kDurable) result |= AMQP::durable;
  if (flags & Queue::Flags::kExclusive) result |= AMQP::exclusive;
  if (flags & Queue::Flags::kAutoDelete) result |= AMQP::autodelete;

  return result;
}

int Convert(utils::Flags<Exchange::Flags> flags) {
  int result = 0;
  if (flags & Exchange::Flags::kPassive) result |= AMQP::passive;
  if (flags & Exchange::Flags::kDurable) result |= AMQP::durable;
  if (flags & Exchange::Flags::kAutoDelete) result |= AMQP::autodelete;
  if (flags & Exchange::Flags::kInternal) result |= AMQP::internal;
  if (flags & Exchange::Flags::kNoWait) result |= AMQP::nowait;

  return result;
}

AMQP::Table CreateHeaders() {
  UASSERT(engine::current_task::IsTaskProcessorThread());

  auto* span = tracing::Span::CurrentSpanUnchecked();
  if (span == nullptr) return {};

  AMQP::Table headers;
  headers["u-trace-id"] = span->GetTraceId();

  return headers;
}

}  // namespace

AmqpChannel::AmqpChannel(AmqpConnection& conn) : conn_{conn} {}

AmqpChannel::~AmqpChannel() = default;

ResponseAwaiter AmqpChannel::DeclareExchange(
    const Exchange& exchange, Exchange::Type exchangeType,
    utils::Flags<Exchange::Flags> flags, engine::Deadline deadline) {
  auto awaiter = conn_.GetAwaiter(deadline);

  {
    auto channel = conn_.GetChannel(deadline);
    awaiter.GetWrapper()->Wrap(channel->declareExchange(
        exchange.GetUnderlying(), Convert(exchangeType), Convert(flags)));
  }

  return awaiter;
}

ResponseAwaiter AmqpChannel::DeclareQueue(const Queue& queue,
                                          utils::Flags<Queue::Flags> flags,
                                          engine::Deadline deadline) {
  auto awaiter = conn_.GetAwaiter(deadline);

  {
    auto channel = conn_.GetChannel(deadline);
    awaiter.GetWrapper()->Wrap(
        channel->declareQueue(queue.GetUnderlying(), Convert(flags)));
  }

  return awaiter;
}

ResponseAwaiter AmqpChannel::BindQueue(const Exchange& exchange,
                                       const Queue& queue,
                                       const std::string& routing_key,
                                       engine::Deadline deadline) {
  auto awaiter = conn_.GetAwaiter(deadline);

  {
    auto channel = conn_.GetChannel(deadline);
    awaiter.GetWrapper()->Wrap(channel->bindQueue(
        exchange.GetUnderlying(), queue.GetUnderlying(), routing_key));
  }

  return awaiter;
}

ResponseAwaiter AmqpChannel::RemoveExchange(const Exchange& exchange,
                                            engine::Deadline deadline) {
  auto awaiter = conn_.GetAwaiter(deadline);

  {
    auto channel = conn_.GetChannel(deadline);
    awaiter.GetWrapper()->Wrap(
        channel->removeExchange(exchange.GetUnderlying()));
  }

  return awaiter;
}

ResponseAwaiter AmqpChannel::RemoveQueue(const Queue& queue,
                                         engine::Deadline deadline) {
  auto awaiter = conn_.GetAwaiter(deadline);

  {
    auto channel = conn_.GetChannel(deadline);
    awaiter.GetWrapper()->Wrap(channel->removeQueue(queue.GetUnderlying()));
  }

  return awaiter;
}

void AmqpChannel::Publish(const Exchange& exchange,
                          const std::string& routing_key,
                          const std::string& message, MessageType type,
                          engine::Deadline deadline) {
  AMQP::Envelope envelope{message.data(), message.size()};
  envelope.setPersistent(type == MessageType::kPersistent);
  envelope.setHeaders(CreateHeaders());

  {
    auto channel = conn_.GetChannel(deadline);

    // We don't care about the result here,
    // even thought publish() could fail synchronously (connection breakage,
    // channel breakage)
    channel->publish(exchange.GetUnderlying(), routing_key, envelope);
  }

  // We don't account publish here, because there's no way to ensure success
}

void AmqpChannel::Ack(uint64_t delivery_tag, engine::Deadline deadline) {
  // No way to acknowledge success, no way to handle synchronous errors
  auto channel = conn_.GetChannel(deadline);
  channel->ack(delivery_tag);
}

void AmqpChannel::Reject(uint64_t delivery_tag, bool requeue,
                         engine::Deadline deadline) {
  // No way to acknowledge success, no way to handle synchronous errors
  auto channel = conn_.GetChannel(deadline);
  channel->reject(delivery_tag, requeue ? AMQP::requeue : 0);
}

void AmqpChannel::SetQos(uint16_t prefetch_count, engine::Deadline deadline) {
  auto deferred = DeferredWrapper::Create();

  {
    auto channel = conn_.GetChannel(deadline);
    deferred->Wrap(channel->setQos(prefetch_count));
  }

  deferred->Wait(deadline);
}

void AmqpChannel::SetupConsumer(const std::string& queue, ErrorCb error_cb,
                                SuccessCb success_cb, MessageCb message_cb,
                                engine::Deadline deadline) {
  auto channel = conn_.GetChannel(deadline);

  channel->onError(error_cb);
  channel->consume(queue)
      .onSuccess(success_cb)
      .onMessage(message_cb)
      .onError(error_cb);
}

void AmqpChannel::CancelConsumer(
    const std::optional<std::string>& consumer_tag) {
  auto channel = conn_.GetChannel({});

  if (consumer_tag.has_value()) {
    channel->cancel(*consumer_tag);
  }
}

void AmqpChannel::AccountMessageConsumed() {
  conn_.GetStatistics().AccountMessageConsumed();
}

AmqpReliableChannel::AmqpReliableChannel(AmqpConnection& conn) : conn_{conn} {}

AmqpReliableChannel::~AmqpReliableChannel() = default;

ResponseAwaiter AmqpReliableChannel::Publish(const Exchange& exchange,
                                             const std::string& routing_key,
                                             const std::string& message,
                                             MessageType type,
                                             engine::Deadline deadline) {
  AMQP::Envelope envelope{message.data(), message.size()};
  envelope.setPersistent(type == MessageType::kPersistent);
  envelope.setHeaders(CreateHeaders());

  auto awaiter = conn_.GetAwaiter(deadline);

  {
    auto reliable = conn_.GetReliableChannel(deadline);

    reliable->publish(exchange.GetUnderlying(), routing_key, envelope)
        .onAck([this, deferred = awaiter.GetWrapper()] {
          AccountMessagePublished();
          deferred->Ok();
        })
        .onError([deferred = awaiter.GetWrapper()](const char* error) {
          deferred->Fail(error);
        });
  }

  return awaiter;
}

void AmqpReliableChannel::AccountMessagePublished() {
  conn_.GetStatistics().AccountMessagePublished();
}

}  // namespace urabbitmq::impl

USERVER_NAMESPACE_END
