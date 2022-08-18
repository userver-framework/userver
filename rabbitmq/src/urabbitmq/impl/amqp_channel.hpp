#pragma once

#include <functional>
#include <memory>

#include <userver/engine/deadline.hpp>
#include <userver/utils/assert.hpp>

#include <userver/urabbitmq/typedefs.hpp>
#include <userver/utils/flags.hpp>

#include <urabbitmq/impl/response_awaiter.hpp>

#include <amqpcpp.h>

USERVER_NAMESPACE_BEGIN

namespace urabbitmq {
class ConsumerBaseImpl;

namespace statistics {
class ConnectionStatistics;
}
}  // namespace urabbitmq

namespace urabbitmq::impl {

class AmqpConnection;
class AmqpReliableChannel;

class AmqpChannel final {
 public:
  AmqpChannel(AmqpConnection& conn);
  ~AmqpChannel();

  ResponseAwaiter DeclareExchange(const Exchange& exchange, Exchange::Type type,
                                  utils::Flags<Exchange::Flags> flags,
                                  engine::Deadline deadline);

  ResponseAwaiter DeclareQueue(const Queue& queue,
                               utils::Flags<Queue::Flags> flags,
                               engine::Deadline deadline);

  ResponseAwaiter BindQueue(const Exchange& exchange, const Queue& queue,
                            const std::string& routing_key,
                            engine::Deadline deadline);

  ResponseAwaiter RemoveExchange(const Exchange& exchange,
                                 engine::Deadline deadline);

  ResponseAwaiter RemoveQueue(const Queue& queue, engine::Deadline deadline);

  void Publish(const Exchange& exchange, const std::string& routing_key,
               const std::string& message, MessageType type,
               engine::Deadline deadline);

  void Ack(uint64_t delivery_tag, engine::Deadline deadline);

  void Reject(uint64_t delivery_tag, bool requeue, engine::Deadline deadline);

  void SetQos(uint16_t prefetch_count, engine::Deadline deadline);

  using ErrorCb = std::function<void(const char*)>;
  using SuccessCb = std::function<void(const std::string&)>;
  using MessageCb = std::function<void(const AMQP::Message&, uint64_t, bool)>;
  void SetupConsumer(const std::string& queue, ErrorCb error_cb,
                     SuccessCb success_cb, MessageCb message_cb,
                     engine::Deadline deadline);

  void CancelConsumer(const std::optional<std::string>& consumer_tag);

 private:
  void AccountMessageConsumed();

  friend class urabbitmq::ConsumerBaseImpl;

  AmqpConnection& conn_;
};

class AmqpReliableChannel final {
 public:
  AmqpReliableChannel(AmqpConnection& conn);
  ~AmqpReliableChannel();

  ResponseAwaiter Publish(const Exchange& exchange,
                          const std::string& routing_key,
                          const std::string& message, MessageType type,
                          engine::Deadline deadline);

 private:
  void AccountMessagePublished();

  AmqpConnection& conn_;
};

}  // namespace urabbitmq::impl

USERVER_NAMESPACE_END
