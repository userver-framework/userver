#pragma once

#include <memory>

#include <userver/engine/deadline.hpp>
#include <userver/utils/assert.hpp>

#include <userver/urabbitmq/typedefs.hpp>
#include <userver/utils/flags.hpp>

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
  AmqpChannel(AmqpConnection& conn, engine::Deadline deadline);
  ~AmqpChannel();

  void DeclareExchange(const Exchange& exchange, Exchange::Type type,
                       utils::Flags<Exchange::Flags> flags,
                       engine::Deadline deadline);

  void DeclareQueue(const Queue& queue, utils::Flags<Queue::Flags> flags,
                    engine::Deadline deadline);

  void BindQueue(const Exchange& exchange, const Queue& queue,
                 const std::string& routing_key, engine::Deadline deadline);

  void RemoveExchange(const Exchange& exchange, engine::Deadline deadline);

  void RemoveQueue(const Queue& queue, engine::Deadline deadline);

  void Publish(const Exchange& exchange, const std::string& routing_key,
               const std::string& message, MessageType type,
               engine::Deadline deadline);

  void Ack(uint64_t delivery_tag, engine::Deadline deadline);

  void Reject(uint64_t delivery_tag, bool requeue, engine::Deadline deadline);

  void SetQos(uint16_t prefetch_count, engine::Deadline deadline);

  void ResetCallbacks();

 private:
  void AccountMessagePublished();
  void AccountMessageConsumed();

  friend class AmqpReliableChannel;
  friend class urabbitmq::ConsumerBaseImpl;

  AmqpConnection& conn_;
  AMQP::Channel channel_;
};

class AmqpReliableChannel final {
 public:
  AmqpReliableChannel(AmqpConnection& conn, engine::Deadline deadline);
  ~AmqpReliableChannel();

  void Publish(const Exchange& exchange, const std::string& routing_key,
               const std::string& message, MessageType type,
               engine::Deadline deadline);

  void ResetCallbacks();

 private:
  AmqpChannel channel_;
  AMQP::Reliable<AMQP::Tagger> reliable_;
};

}  // namespace urabbitmq::impl

USERVER_NAMESPACE_END
