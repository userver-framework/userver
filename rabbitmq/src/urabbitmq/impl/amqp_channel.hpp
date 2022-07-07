#pragma once

#include <memory>

#include <engine/ev/thread_control.hpp>

#include <userver/urabbitmq/typedefs.hpp>
#include <userver/urabbitmq/exchange_type.hpp>

#include <amqpcpp.h>

USERVER_NAMESPACE_BEGIN

namespace urabbitmq {
class ConsumerBaseImpl;
}

namespace urabbitmq::impl {

class IAmqpChannel {
 public:
  virtual ~IAmqpChannel() = default;

  virtual void DeclareExchange(const Exchange&, ExchangeType) {}

  virtual void DeclareQueue(const Queue&) {}

  virtual void BindQueue(const Exchange&, const Queue&,
                 const std::string&) {}

  virtual void Publish(const Exchange&, const std::string&, const std::string&) {}
};

class AmqpConnection;
class AmqpReliableChannel;

class AmqpChannel final : public IAmqpChannel {
 public:
  AmqpChannel(AmqpConnection& conn);
  ~AmqpChannel() override;

  void DeclareExchange(const Exchange& exchange, ExchangeType type) override;

  void DeclareQueue(const Queue& queue) override;

  void BindQueue(const Exchange& exchange, const Queue& queue,
                 const std::string& routing_key) override;

  void Publish(const Exchange& exchange, const std::string& routing_key, const std::string& message) override;

  engine::ev::ThreadControl& GetEvThread();

 private:
  friend class AmqpReliableChannel;
  friend class urabbitmq::ConsumerBaseImpl;
  engine::ev::ThreadControl thread_;

  std::unique_ptr<AMQP::Channel> channel_;
};

class AmqpReliableChannel final : public IAmqpChannel {
 public:
  AmqpReliableChannel(AmqpConnection& conn);
  ~AmqpReliableChannel() override;

  void Publish(const Exchange& exchange, const std::string& routing_key, const std::string& message) override;

 private:
  AmqpChannel channel_;

  std::unique_ptr<AMQP::Reliable<AMQP::Tagger>> reliable_;
};

}

USERVER_NAMESPACE_END
