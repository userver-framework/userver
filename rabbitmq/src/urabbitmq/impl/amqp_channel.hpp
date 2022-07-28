#pragma once

#include <memory>

#include <userver/engine/deadline.hpp>
#include <userver/utils/assert.hpp>

#include <engine/ev/thread_control.hpp>

#include <userver/urabbitmq/exchange_type.hpp>
#include <userver/urabbitmq/typedefs.hpp>
#include <userver/utils/flags.hpp>

#include <amqpcpp.h>

USERVER_NAMESPACE_BEGIN

namespace urabbitmq {
class ConsumerBaseImpl;
}

namespace urabbitmq::impl {

class IAmqpChannel {
 public:
  virtual ~IAmqpChannel() = default;

  virtual void DeclareExchange(const Exchange&, ExchangeType,
                               utils::Flags<Exchange::Flags>,
                               engine::Deadline) {
    UASSERT_MSG(false, "One shouldn't end up here.");
  }

  virtual void DeclareQueue(const Queue&, utils::Flags<Queue::Flags>,
                            engine::Deadline) {
    UASSERT_MSG(false, "One shouldn't end up here.");
  }

  virtual void BindQueue(const Exchange&, const Queue&, const std::string&,
                         engine::Deadline) {
    UASSERT_MSG(false, "One shouldn't end up here.");
  }

  virtual void RemoveExchange(const Exchange&, engine::Deadline) {
    UASSERT_MSG(false, "One shouldn't end up here.");
  }

  virtual void RemoveQueue(const Queue&, engine::Deadline) {
    UASSERT_MSG(false, "One shouldn't end up here.");
  }

  virtual void Publish(const Exchange&, const std::string&, const std::string&,
                       MessageType, engine::Deadline) {
    UASSERT_MSG(false, "One shouldn't end up here.");
  }

  virtual void ResetCallbacks() {
    UASSERT_MSG(false, "One shouldn't end up here.");
  }

  virtual bool Broken() const = 0;
};

class AmqpConnection;
class AmqpReliableChannel;

class AmqpChannel final : public IAmqpChannel {
 public:
  AmqpChannel(AmqpConnection& conn, engine::Deadline deadline);
  ~AmqpChannel() override;

  void DeclareExchange(const Exchange& exchange, ExchangeType type,
                       utils::Flags<Exchange::Flags> flags,
                       engine::Deadline deadline) override;

  void DeclareQueue(const Queue& queue, utils::Flags<Queue::Flags> flags,
                    engine::Deadline deadline) override;

  void BindQueue(const Exchange& exchange, const Queue& queue,
                 const std::string& routing_key,
                 engine::Deadline deadline) override;

  void RemoveExchange(const Exchange& exchange,
                      engine::Deadline deadline) override;

  void RemoveQueue(const Queue& queue, engine::Deadline deadline) override;

  void Publish(const Exchange& exchange, const std::string& routing_key,
               const std::string& message, MessageType type,
               engine::Deadline deadline) override;

  void ResetCallbacks() override;

  bool Broken() const override;

 private:
  engine::ev::ThreadControl& GetEvThread();

  void Ack(uint64_t delivery_tag);
  void Reject(uint64_t delivery_tag, bool requeue);

  class BrokenGuard final {
   public:
    BrokenGuard(AmqpChannel* parent);
    ~BrokenGuard();

   private:
    bool& broken_;
    int exceptions_on_enter_;
  };
  BrokenGuard GetExceptionsGuard();

  friend class AmqpReliableChannel;
  friend class urabbitmq::ConsumerBaseImpl;
  engine::ev::ThreadControl thread_;

  std::unique_ptr<AMQP::Channel> channel_;
  bool broken_{false};
};

class AmqpReliableChannel final : public IAmqpChannel {
 public:
  AmqpReliableChannel(AmqpConnection& conn, engine::Deadline deadline);
  ~AmqpReliableChannel() override;

  void Publish(const Exchange& exchange, const std::string& routing_key,
               const std::string& message, MessageType type,
               engine::Deadline deadline) override;

  void ResetCallbacks() override;

  bool Broken() const override;

 private:
  AmqpChannel channel_;

  std::unique_ptr<AMQP::Reliable<AMQP::Tagger>> reliable_;
};

}  // namespace urabbitmq::impl

USERVER_NAMESPACE_END
