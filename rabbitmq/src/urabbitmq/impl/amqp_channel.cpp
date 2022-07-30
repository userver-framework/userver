#include "amqp_channel.hpp"

#include <optional>

#include <userver/engine/task/task.hpp>
#include <userver/tracing/span.hpp>

#include <urabbitmq/impl/amqp_connection.hpp>
#include <urabbitmq/impl/deferred_wrapper.hpp>

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

}  // namespace

AmqpChannel::AmqpChannel(AmqpConnection& conn, engine::Deadline deadline)
    : thread_{conn.GetEvThread()} {
  auto deferred = DeferredWrapper::Create();

  thread_.RunInEvLoopDeferred([this, &conn, deferred] {
    try {
      channel_ = std::make_unique<AMQP::Channel>(&conn.GetNative());
    } catch (const std::exception& ex) {
      deferred->Fail(ex.what());
      return;
    }
    channel_->onReady([deferred = deferred] { deferred->Ok(); });
    channel_->onError(
        [deferred = deferred](const char* error) { deferred->Fail(error); });
  });

  deferred->Wait(deadline);
}

AmqpChannel::~AmqpChannel() {
  thread_.RunInEvLoopSync([channel = std::move(channel_)]() mutable {
    // We have to reset callbacks here, otherwise unexpected bad things
    // might happen when RMQ sends us a channel close frame: if we capture
    // [this] in channel.onError (and we do in consumer) it crashes.
    // We could probably just use shared_pointers more carefully, but this
    // is a good safety measure anyway
    channel->onError({});
    channel->onReady({});
    channel.reset();
  });
}

AmqpChannel::BrokenGuard::BrokenGuard(AmqpChannel* parent)
    : broken_{parent->broken_},
      exceptions_on_enter_{std::uncaught_exceptions()} {}

AmqpChannel::BrokenGuard::~BrokenGuard() {
  if (std::uncaught_exceptions() != exceptions_on_enter_) {
    broken_ = true;
  }
}

void AmqpChannel::DeclareExchange(const Exchange& exchange,
                                  Exchange::Type exchangeType,
                                  utils::Flags<Exchange::Flags> flags,
                                  engine::Deadline deadline) {
  auto guard = GetExceptionsGuard();
  auto deferred = DeferredWrapper::Create();

  thread_.RunInEvLoopDeferred([this, exchange = exchange.GetUnderlying(),
                               exchangeType, flags = Convert(flags), deferred] {
    deferred->Wrap(
        channel_->declareExchange(exchange, Convert(exchangeType), flags));
  });

  deferred->Wait(deadline);
}

void AmqpChannel::DeclareQueue(const Queue& queue,
                               utils::Flags<Queue::Flags> flags,
                               engine::Deadline deadline) {
  auto guard = GetExceptionsGuard();
  auto deferred = DeferredWrapper::Create();

  thread_.RunInEvLoopDeferred(
      [this, queue = queue.GetUnderlying(), flags = Convert(flags), deferred] {
        deferred->Wrap(channel_->declareQueue(queue, flags));
      });

  deferred->Wait(deadline);
}

void AmqpChannel::BindQueue(const Exchange& exchange, const Queue& queue,
                            const std::string& routing_key,
                            engine::Deadline deadline) {
  auto guard = GetExceptionsGuard();
  auto deferred = DeferredWrapper::Create();

  thread_.RunInEvLoopDeferred([this, exchange = exchange.GetUnderlying(),
                               queue = queue.GetUnderlying(), routing_key,
                               deferred] {
    deferred->Wrap(channel_->bindQueue(exchange, queue, routing_key));
  });

  deferred->Wait(deadline);
}

void AmqpChannel::RemoveExchange(const Exchange& exchange,
                                 engine::Deadline deadline) {
  auto guard = GetExceptionsGuard();
  auto deferred = DeferredWrapper::Create();

  thread_.RunInEvLoopDeferred(
      [this, exchange = exchange.GetUnderlying(), deferred] {
        deferred->Wrap(channel_->removeExchange(exchange));
      });

  deferred->Wait(deadline);
}

void AmqpChannel::RemoveQueue(const Queue& queue, engine::Deadline deadline) {
  auto guard = GetExceptionsGuard();
  auto deferred = DeferredWrapper::Create();

  thread_.RunInEvLoopDeferred([this, queue = queue.GetUnderlying(), deferred] {
    deferred->Wrap(channel_->removeQueue(queue));
  });

  deferred->Wait(deadline);
}

void AmqpChannel::Publish(const Exchange& exchange,
                          const std::string& routing_key,
                          const std::string& message, MessageType type,
                          engine::Deadline) {
  // We don't care about the result here,
  // even thought publish() could fail synchronously (connection breakage,
  // channel breakage)
  thread_.RunInEvLoopDeferred([this, exchange = exchange.GetUnderlying(),
                               routing_key, message, headers = CreateHeaders(),
                               type] {
    AMQP::Envelope envelope{message.data(), message.size()};
    envelope.setPersistent(type == MessageType::kPersistent);
    envelope.setHeaders(std::move(headers));

    channel_->publish(exchange, routing_key, envelope);
  });
}

void AmqpChannel::ResetCallbacks() {
  thread_.RunInEvLoopSync([this] {
    channel_->onError({});
    channel_->onReady({});
  });
}

bool AmqpChannel::Broken() const { return broken_; }

engine::ev::ThreadControl& AmqpChannel::GetEvThread() { return thread_; }

void AmqpChannel::Ack(uint64_t delivery_tag) {
  // No way to acknowledge success, no way to handle synchronous errors,
  // thus Deferred and not Sync
  thread_.RunInEvLoopDeferred(
      [this, delivery_tag] { channel_->ack(delivery_tag); });
}

void AmqpChannel::Reject(uint64_t delivery_tag, bool requeue) {
  // No way to acknowledge success, no way to handle synchronous errors,
  // thus Deferred and not Sync
  thread_.RunInEvLoopDeferred([this, delivery_tag, requeue] {
    channel_->reject(delivery_tag, requeue ? AMQP::requeue : 0);
  });
}

AmqpChannel::BrokenGuard AmqpChannel::GetExceptionsGuard() { return {this}; }

AmqpReliableChannel::AmqpReliableChannel(AmqpConnection& conn,
                                         engine::Deadline deadline)
    : channel_{conn, deadline} {
  channel_.GetEvThread().RunInEvLoopSync([this] {
    try {
      reliable_ =
          std::make_unique<AMQP::Reliable<AMQP::Tagger>>(*channel_.channel_);
    } catch (const std::exception&) {
    }
  });

  if (!reliable_) {
    throw std::runtime_error{"failed"};
  }
}

AmqpReliableChannel::~AmqpReliableChannel() {
  channel_.GetEvThread().RunInEvLoopSync(
      [this, reliable = std::move(reliable_)]() mutable { reliable_.reset(); });
}

void AmqpReliableChannel::Publish(const Exchange& exchange,
                                  const std::string& routing_key,
                                  const std::string& message, MessageType type,
                                  engine::Deadline deadline) {
  auto guard = channel_.GetExceptionsGuard();
  auto deferred = DeferredWrapper::Create();

  channel_.thread_.RunInEvLoopDeferred(
      [this, exchange = exchange.GetUnderlying(), routing_key, message,
       headers = CreateHeaders(), type, deferred] {
        AMQP::Envelope envelope{message.data(), message.size()};
        envelope.setPersistent(type == MessageType::kPersistent);
        envelope.setHeaders(std::move(headers));

        reliable_->publish(exchange, routing_key, envelope)
            .onAck([deferred] { deferred->Ok(); })
            .onError([deferred](const char* error) { deferred->Fail(error); });
      });

  deferred->Wait(deadline);
}

void AmqpReliableChannel::ResetCallbacks() { channel_.ResetCallbacks(); }

bool AmqpReliableChannel::Broken() const { return channel_.Broken(); }

}  // namespace urabbitmq::impl

USERVER_NAMESPACE_END