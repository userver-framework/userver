#include "amqp_channel.hpp"

#include <optional>

#include <userver/engine/single_consumer_event.hpp>

#include <urabbitmq/impl/amqp_connection.hpp>
#include <urabbitmq/impl/deferred_wrapper.hpp>

USERVER_NAMESPACE_BEGIN

namespace urabbitmq::impl {

namespace {

AMQP::ExchangeType Convert(urabbitmq::ExchangeType type) {
  using From = urabbitmq::ExchangeType;
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

}  // namespace

AmqpChannel::AmqpChannel(AmqpConnection& conn) : thread_{conn.GetEvThread()} {
  auto deferred = DeferredWrapper::Create();

  thread_.RunInEvLoopSync([this, &conn, deferred] {
    channel_ = std::make_unique<AMQP::Channel>(&conn.GetNative());
    channel_->onReady([deferred = deferred] { deferred->Ok(); });
    channel_->onError(
        [deferred = deferred](const char* error) { deferred->Fail(error); });
  });

  deferred->Wait();
}

AmqpChannel::~AmqpChannel() {
  thread_.RunInEvLoopSync(
      [channel = std::move(channel_)]() mutable { channel.reset(); });
}

void AmqpChannel::DeclareExchange(const Exchange& exchange,
                                  ExchangeType exchangeType) {
  auto deferred = DeferredWrapper::Create();

  thread_.RunInEvLoopAsync([this, exchange = exchange.GetUnderlying(),
                            exchangeType, deferred] {
    deferred->Wrap(channel_->declareExchange(exchange, Convert(exchangeType)));
  });

  deferred->Wait();
}

void AmqpChannel::DeclareQueue(const Queue& queue) {
  auto deferred = DeferredWrapper::Create();

  thread_.RunInEvLoopAsync([this, queue = queue.GetUnderlying(), deferred] {
    deferred->Wrap(channel_->declareQueue(queue));
  });

  deferred->Wait();
}

void AmqpChannel::BindQueue(const Exchange& exchange, const Queue& queue,
                            const std::string& routing_key) {
  auto deferred = DeferredWrapper::Create();

  thread_.RunInEvLoopAsync([this, exchange = exchange.GetUnderlying(),
                            queue = queue.GetUnderlying(), routing_key,
                            deferred] {
    deferred->Wrap(channel_->bindQueue(exchange, queue, routing_key));
  });

  deferred->Wait();
}

void AmqpChannel::Publish(const Exchange& exchange,
                          const std::string& routing_key,
                          const std::string& message) {
  auto deferred = DeferredWrapper::Create();

  thread_.RunInEvLoopAsync([this, exchange = exchange.GetUnderlying(),
                            routing_key, message, deferred] {
    if (channel_->publish(exchange, routing_key, message)) {
      deferred->Ok();
    } else {
      deferred->Fail("publish failed");
    }
  });

  deferred->Wait();
}

void AmqpChannel::ResetCallbacks() {
  thread_.RunInEvLoopSync([this] {
    channel_->onError({});
    channel_->onReady({});
  });
}

engine::ev::ThreadControl& AmqpChannel::GetEvThread() { return thread_; }

void AmqpChannel::Cancel(const std::string& consumer_tag) {
  auto deferred = DeferredWrapper::Create();

  thread_.RunInEvLoopSync([this, consumer_tag, deferred] {
    deferred->Wrap(channel_->cancel(consumer_tag));
  });

  deferred->Wait();
}

void AmqpChannel::Ack(uint64_t delivery_tag) {
  thread_.RunInEvLoopAsync(
      [this, delivery_tag] { channel_->ack(delivery_tag); });
}

void AmqpChannel::Reject(uint64_t delivery_tag, bool requeue) {
  thread_.RunInEvLoopAsync([this, delivery_tag, requeue] {
    channel_->reject(delivery_tag, requeue ? AMQP::requeue : 0);
  });
}

AmqpReliableChannel::AmqpReliableChannel(AmqpConnection& conn)
    : channel_{conn} {
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
                                  const std::string& message) {
  auto deferred = DeferredWrapper::Create();

  channel_.thread_.RunInEvLoopAsync([this, exchange = exchange.GetUnderlying(),
                                     routing_key, message, deferred] {
    reliable_->publish(exchange, routing_key, message)
        .onAck([deferred] { deferred->Ok(); })
        .onError([deferred](const char* error) { deferred->Fail(error); });
  });

  deferred->Wait();
}

void AmqpReliableChannel::ResetCallbacks() { channel_.ResetCallbacks(); }

}  // namespace urabbitmq::impl

USERVER_NAMESPACE_END