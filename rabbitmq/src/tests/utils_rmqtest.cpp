#include "utils_rmqtest.hpp"

#include <userver/utils/from_string.hpp>
#include <userver/utils/uuid4.hpp>

USERVER_NAMESPACE_BEGIN

namespace {

constexpr const char* kTestsuiteRabbitMqPort = "TESTSUITE_RABBITMQ_TCP_PORT";
constexpr std::uint16_t kDefaultRabbitMqPort = 19002;

clients::dns::Resolver CreateResolver() {
  return clients::dns::Resolver{engine::current_task::GetTaskProcessor(), {}};
}

std::shared_ptr<urabbitmq::Client> CreateClient(
    userver::clients::dns::Resolver& resolver) {
  urabbitmq::AuthSettings auth{};

  urabbitmq::EndpointInfo endpoint{};
  endpoint.port = GetRabbitMqPort();
  // endpoint.host = "192.168.1.4";

  urabbitmq::RabbitEndpoints endpoints;
  endpoints.auth = std::move(auth);
  endpoints.endpoints = {std::move(endpoint)};

  urabbitmq::ClientSettings settings;
  settings.ev_pool_type = urabbitmq::EvPoolType::kOwned;
  settings.thread_count = 2;
  settings.channels_per_connection = 1;
  settings.channels_per_connection = 10;
  settings.secure = false;
  settings.endpoints = std::move(endpoints);

  return urabbitmq::Client::Create(resolver, settings);
}

}  // namespace

uint16_t GetRabbitMqPort() {
  const auto* rabbitmq_port_env = std::getenv(kTestsuiteRabbitMqPort);

  return rabbitmq_port_env ? utils::FromString<std::uint16_t>(rabbitmq_port_env)
                           : kDefaultRabbitMqPort;
}

ClientWrapper::ClientWrapper()
    : resolver_{CreateResolver()},
      client_{CreateClient(resolver_)},
      exchange_{utils::generators::GenerateUuid()},
      queue_{utils::generators::GenerateUuid()},
      routing_key_{utils::generators::GenerateUuid()},
      deadline_{engine::Deadline::FromDuration(utest::kMaxTestWaitTime)} {}

ClientWrapper::~ClientWrapper() {
  try {
    auto channel = client_->GetAdminChannel();
    channel.RemoveExchange(GetExchange(), GetDeadline());
    channel.RemoveQueue(GetQueue(), GetDeadline());
  } catch (const std::exception&) {
  }
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
  channel.DeclareExchange(GetExchange(), urabbitmq::Exchange::Type::kFanOut, {},
                          GetDeadline());
  channel.DeclareQueue(GetQueue(), {}, GetDeadline());
  channel.BindQueue(GetExchange(), GetQueue(), GetRoutingKey(), GetDeadline());
}

USERVER_NAMESPACE_END
