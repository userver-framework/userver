#include "utils_rmqtest.hpp"

#include <userver/utils/from_string.hpp>
#include <userver/utils/uuid4.hpp>

USERVER_NAMESPACE_BEGIN

namespace {

constexpr const char* kTestsuiteRabbitMqPort = "TESTSUITE_RABBITMQ_TCP_PORT";
constexpr std::uint16_t kDefaultRabbitMqPort = 19002;

uint16_t GetRabbitMqPort() {
  const auto* rabbitmq_port_env = std::getenv(kTestsuiteRabbitMqPort);

  return rabbitmq_port_env ? utils::FromString<std::uint16_t>(rabbitmq_port_env)
                           : kDefaultRabbitMqPort;
}

clients::dns::Resolver CreateResolver() {
  return clients::dns::Resolver{engine::current_task::GetTaskProcessor(), {}};
}

std::shared_ptr<urabbitmq::Client> CreateClient(
    userver::clients::dns::Resolver& resolver) {
  urabbitmq::AuthSettings auth{};
  // TODO : https://github.com/userver-framework/userver/issues/52
  // this might get stuck in a loop for now
  // auth.secure = true;

  urabbitmq::EndpointInfo endpoint{};
  endpoint.port = GetRabbitMqPort();

  const urabbitmq::ClientSettings settings{urabbitmq::EvPoolType::kOwned,
                                           2,
                                           1,
                                           10,
                                           {std::move(endpoint)},
                                           std::move(auth)};

  return std::make_shared<urabbitmq::Client>(resolver, settings);
}

}  // namespace

ClientWrapper::ClientWrapper()
    : resolver_{CreateResolver()},
      client_{CreateClient(resolver_)},
      exchange_{utils::generators::GenerateUuid()},
      queue_{utils::generators::GenerateUuid()},
      routing_key_{utils::generators::GenerateUuid()},
      deadline_{engine::Deadline::FromDuration(utest::kMaxTestWaitTime)} {}

ClientWrapper::~ClientWrapper() {
  auto channel = client_->GetAdminChannel();
  channel.RemoveExchange(GetExchange(), GetDeadline());
  channel.RemoveQueue(GetQueue(), GetDeadline());
}

urabbitmq::Client* ClientWrapper::operator->() const { return client_.get(); }

urabbitmq::Client& ClientWrapper::operator*() const { return *client_; }

std::shared_ptr<urabbitmq::Client> ClientWrapper::Get() const {
  return client_;
}

const urabbitmq::Exchange& ClientWrapper::GetExchange() const {
  return exchange_;
}

const urabbitmq::Queue& ClientWrapper::GetQueue() const { return queue_; }

const std::string& ClientWrapper::GetRoutingKey() const { return routing_key_; }

engine::Deadline ClientWrapper::GetDeadline() const { return deadline_; }

void ClientWrapper::SetupRmqEntities() const {
  auto channel = client_->GetAdminChannel();
  channel.DeclareExchange(GetExchange(), urabbitmq::ExchangeType::kFanOut, {},
                          GetDeadline());
  channel.DeclareQueue(GetQueue(), {}, GetDeadline());
  channel.BindQueue(GetExchange(), GetQueue(), GetRoutingKey(), GetDeadline());
}

USERVER_NAMESPACE_END
