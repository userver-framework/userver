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
  UASSERT(engine::current_task::GetTaskProcessorOptional() != nullptr);

  auto* span = tracing::Span::CurrentSpanUnchecked();
  if (span == nullptr) return {};

  AMQP::Table headers;
  headers["u-trace-id"] = span->GetTraceId();

  return headers;
}

AMQP::Channel CreateChannel(AmqpConnection& conn, engine::Deadline deadline) {
  auto lock = conn.Lock(deadline);
  conn.SetOperationDeadline(deadline);
  return {&conn.GetNative()};
}

AMQP::Reliable<AMQP::Tagger> CreateReliable(AmqpConnection& conn,
                                            AMQP::Channel& channel,
                                            engine::Deadline deadline) {
  auto lock = conn.Lock(deadline);
  conn.SetOperationDeadline(deadline);
  return {channel};
}

}  // namespace

AmqpChannel::AmqpChannel(AmqpConnection& conn, engine::Deadline deadline)
    : conn_{conn}, channel_{CreateChannel(conn_, deadline)} {
  auto deferred = DeferredWrapper::Create();

  const auto fn = [this, deferred] {
    channel_.onReady([deferred = deferred] { deferred->Ok(); });
    channel_.onError(
        [deferred = deferred](const char* error) { deferred->Fail(error); });
  };
  conn_.RunLocked(fn, deadline);

  deferred->Wait(deadline);
}

AmqpChannel::~AmqpChannel() { ResetCallbacks(); }

void AmqpChannel::DeclareExchange(const Exchange& exchange,
                                  Exchange::Type exchangeType,
                                  utils::Flags<Exchange::Flags> flags,
                                  engine::Deadline deadline) {
  auto deferred = DeferredWrapper::Create();

  const auto fn = [this, &exchange, type = Convert(exchangeType),
                   flags = Convert(flags)]() -> decltype(auto) {
    return channel_.declareExchange(exchange.GetUnderlying(), type, flags);
  };
  deferred->Wrap(conn_.RunLocked(fn, deadline));

  deferred->Wait(deadline);
}

void AmqpChannel::DeclareQueue(const Queue& queue,
                               utils::Flags<Queue::Flags> flags,
                               engine::Deadline deadline) {
  auto deferred = DeferredWrapper::Create();

  const auto fn = [this, &queue, flags = Convert(flags)]() -> decltype(auto) {
    return channel_.declareQueue(queue.GetUnderlying(), flags);
  };
  deferred->Wrap(conn_.RunLocked(fn, deadline));

  deferred->Wait(deadline);
}

void AmqpChannel::BindQueue(const Exchange& exchange, const Queue& queue,
                            const std::string& routing_key,
                            engine::Deadline deadline) {
  auto deferred = DeferredWrapper::Create();

  const auto fn = [this, &exchange, &queue, &routing_key]() -> decltype(auto) {
    return channel_.bindQueue(exchange.GetUnderlying(), queue.GetUnderlying(),
                              routing_key);
  };
  deferred->Wrap(conn_.RunLocked(fn, deadline));

  deferred->Wait(deadline);
}

void AmqpChannel::RemoveExchange(const Exchange& exchange,
                                 engine::Deadline deadline) {
  auto deferred = DeferredWrapper::Create();

  const auto fn = [this, &exchange]() -> decltype(auto) {
    return channel_.removeExchange(exchange.GetUnderlying());
  };
  deferred->Wrap(conn_.RunLocked(fn, deadline));

  deferred->Wait(deadline);
}

void AmqpChannel::RemoveQueue(const Queue& queue, engine::Deadline deadline) {
  auto deferred = DeferredWrapper::Create();

  const auto fn = [this, &queue]() -> decltype(auto) {
    return channel_.removeQueue(queue.GetUnderlying());
  };
  deferred->Wrap(conn_.RunLocked(fn, deadline));

  deferred->Wait(deadline);
}

void AmqpChannel::Publish(const Exchange& exchange,
                          const std::string& routing_key,
                          const std::string& message, MessageType type,
                          engine::Deadline deadline) {
  AMQP::Envelope envelope{message.data(), message.size()};
  envelope.setPersistent(type == MessageType::kPersistent);
  envelope.setHeaders(CreateHeaders());

  // We don't care about the result here,
  // even thought publish() could fail synchronously (connection breakage,
  // channel breakage)
  const auto fn = [this, &exchange, &routing_key, &envelope] {
    channel_.publish(exchange.GetUnderlying(), routing_key, envelope);
  };
  conn_.RunLocked(fn, deadline);

  // We don't account publish here, because there's no way to ensure success
}

void AmqpChannel::ResetCallbacks() {
  const auto fn = [this] {
    channel_.onError({});
    channel_.onReady({});
  };
  conn_.RunLocked(fn, {});
}

void AmqpChannel::Ack(uint64_t delivery_tag, engine::Deadline deadline) {
  // No way to acknowledge success, no way to handle synchronous errors
  const auto fn = [this, delivery_tag] { channel_.ack(delivery_tag); };
  conn_.RunLocked(fn, deadline);
}

void AmqpChannel::Reject(uint64_t delivery_tag, bool requeue,
                         engine::Deadline deadline) {
  // No way to acknowledge success, no way to handle synchronous errors
  const auto fn = [this, delivery_tag, requeue] {
    channel_.reject(delivery_tag, requeue ? AMQP::requeue : 0);
  };
  conn_.RunLocked(fn, deadline);
}

void AmqpChannel::SetQos(uint16_t prefetch_count, engine::Deadline deadline) {
  auto deferred = DeferredWrapper::Create();

  const auto fn = [this, prefetch_count]() -> decltype(auto) {
    return channel_.setQos(prefetch_count);
  };
  deferred->Wrap(conn_.RunLocked(fn, deadline));

  deferred->Wait(deadline);
}

void AmqpChannel::AccountMessagePublished() {
  conn_.GetStatistics().AccountMessagePublished();
}

void AmqpChannel::AccountMessageConsumed() {
  conn_.GetStatistics().AccountMessageConsumed();
}

AmqpReliableChannel::AmqpReliableChannel(AmqpConnection& conn,
                                         engine::Deadline deadline)
    : channel_{conn, deadline},
      reliable_{CreateReliable(conn, channel_.channel_, deadline)} {}

AmqpReliableChannel::~AmqpReliableChannel() = default;

void AmqpReliableChannel::Publish(const Exchange& exchange,
                                  const std::string& routing_key,
                                  const std::string& message, MessageType type,
                                  engine::Deadline deadline) {
  auto deferred = DeferredWrapper::Create();

  AMQP::Envelope envelope{message.data(), message.size()};
  envelope.setPersistent(type == MessageType::kPersistent);
  envelope.setHeaders(CreateHeaders());

  const auto fn = [this, deferred, &exchange, &routing_key, &envelope] {
    reliable_.publish(exchange.GetUnderlying(), routing_key, envelope)
        .onAck([deferred] { deferred->Ok(); })
        .onError([deferred](const char* error) { deferred->Fail(error); });
  };
  channel_.conn_.RunLocked(fn, deadline);

  deferred->Wait(deadline);

  channel_.AccountMessagePublished();
}

void AmqpReliableChannel::ResetCallbacks() { channel_.ResetCallbacks(); }

}  // namespace urabbitmq::impl

USERVER_NAMESPACE_END