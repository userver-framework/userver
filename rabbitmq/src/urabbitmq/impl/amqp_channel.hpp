#pragma once

#include <memory>

#include <userver/utils/assert.hpp>

#include <engine/ev/thread_control.hpp>

#include <userver/urabbitmq/exchange_type.hpp>
#include <userver/urabbitmq/typedefs.hpp>

#include <amqpcpp.h>

USERVER_NAMESPACE_BEGIN

namespace urabbitmq {
class ConsumerBaseImpl;
}

namespace urabbitmq::impl {

class IAmqpChannel {
 public:
  virtual ~IAmqpChannel() = default;

  virtual void DeclareExchange(const Exchange&, ExchangeType) {
    UASSERT_MSG(false, "One shouldn't end up here.");
  }

  virtual void DeclareQueue(const Queue&) {
    UASSERT_MSG(false, "One shouldn't end up here.");
  }

  virtual void BindQueue(const Exchange&, const Queue&, const std::string&) {
    UASSERT_MSG(false, "One shouldn't end up here.");
  }

  virtual void RemoveExchange(const Exchange&) {
    UASSERT_MSG(false, "One shouldn't end up here.");
  }

  virtual void RemoveQueue(const Queue&) {
    UASSERT_MSG(false, "One shouldn't end up here.");
  }

  virtual void Publish(const Exchange&, const std::string&,
                       const std::string&) {
    UASSERT_MSG(false, "One shouldn't end up here.");
  }

  virtual void ResetCallbacks() {
    UASSERT_MSG(false, "One shouldn't end up here.");
  }
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

  void RemoveExchange(const Exchange& exchange) override;

  void RemoveQueue(const Queue& queue) override;

  void Publish(const Exchange& exchange, const std::string& routing_key,
               const std::string& message) override;

  void ResetCallbacks() override;

 private:
  engine::ev::ThreadControl& GetEvThread();

  void Ack(uint64_t delivery_tag);
  void Reject(uint64_t delivery_tag, bool requeue);

  friend class AmqpReliableChannel;
  friend class urabbitmq::ConsumerBaseImpl;
  engine::ev::ThreadControl thread_;

  std::unique_ptr<AMQP::Channel> channel_;
};

class AmqpReliableChannel final : public IAmqpChannel {
 public:
  AmqpReliableChannel(AmqpConnection& conn);
  ~AmqpReliableChannel() override;

  void Publish(const Exchange& exchange, const std::string& routing_key,
               const std::string& message) override;

  void ResetCallbacks() override;

 private:
  AmqpChannel channel_;

  std::unique_ptr<AMQP::Reliable<AMQP::Tagger>> reliable_;
};

}  // namespace urabbitmq::impl

USERVER_NAMESPACE_END
