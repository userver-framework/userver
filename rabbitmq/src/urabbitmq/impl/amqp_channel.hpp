#pragma once

#include <memory>

#include <engine/ev/thread_control.hpp>

#include <userver/urabbitmq/typedefs.hpp>
#include <userver/urabbitmq/exchange_type.hpp>

#include <amqpcpp.h>

USERVER_NAMESPACE_BEGIN

namespace urabbitmq::impl {

class AmqpConnection;
class AmqpReliableChannel;

class AmqpChannel final {
 public:
  AmqpChannel(AmqpConnection& conn);
  ~AmqpChannel();

  void DeclareExchange(const Exchange& exchange, ExchangeType type);

  void DeclareQueue(const Queue& queue);

  void BindQueue(const Exchange& exchange, const Queue& queue,
                 const std::string& routing_key);

  void Publish(const Exchange& exchange, const std::string& routing_key, const std::string& message);

 private:
  friend class AmqpReliableChannel;
  engine::ev::ThreadControl thread_;

  std::unique_ptr<AMQP::Channel> channel_;
};

class AmqpReliableChannel final {
 public:
  AmqpReliableChannel(AmqpConnection& conn);
  ~AmqpReliableChannel();

  void Publish(const Exchange& exchange, const std::string& routing_key, const std::string& message);

 private:
  AmqpChannel channel_;

  std::unique_ptr<AMQP::Reliable<AMQP::Tagger>> reliable_;
};

}

USERVER_NAMESPACE_END
