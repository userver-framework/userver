#pragma once

#include <userver/utest/utest.hpp>

#include <userver/clients/dns/resolver.hpp>
#include <userver/rabbitmq.hpp>

USERVER_NAMESPACE_BEGIN

uint16_t GetRabbitMqPort();

class ClientWrapper final {
 public:
  ClientWrapper();
  ~ClientWrapper();

  urabbitmq::Client& operator*() const;

  urabbitmq::Client* operator->() const;

  std::shared_ptr<urabbitmq::Client> Get() const;

  const urabbitmq::Exchange& GetExchange() const;

  const urabbitmq::Queue& GetQueue() const;

  const std::string& GetRoutingKey() const;

  void SetupRmqEntities() const;

  engine::Deadline GetDeadline() const;

 private:
  clients::dns::Resolver resolver_;
  std::shared_ptr<urabbitmq::Client> client_;

  const urabbitmq::Exchange exchange_;
  const urabbitmq::Queue queue_;
  const std::string routing_key_;

  const engine::Deadline deadline_;
  mutable urabbitmq::AdminChannel admin_;
};

namespace urabbitmq {

class TestsHelper final {
 public:
  static ClientSettings CreateSettings();
};

}  // namespace urabbitmq

USERVER_NAMESPACE_END
