#include "utils_rmqtest.hpp"

#include <userver/utils/assert.hpp>
#include <userver/utils/from_string.hpp>
#include <userver/utils/uuid4.hpp>

USERVER_NAMESPACE_BEGIN

namespace {

constexpr const char* kTestsuiteRabbitMqPort = "TESTSUITE_RABBITMQ_TCP_PORT";
constexpr std::uint16_t kDefaultRabbitMqPort = 8672;

clients::dns::Resolver CreateResolver() {
  return clients::dns::Resolver{engine::current_task::GetTaskProcessor(), {}};
}

std::shared_ptr<urabbitmq::Client> CreateClient(
    clients::dns::Resolver& resolver) {
  return urabbitmq::Client::Create(resolver,
                                   urabbitmq::TestsHelper::CreateSettings());
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
      deadline_{engine::Deadline::FromDuration(utest::kMaxTestWaitTime)},
      admin_{client_->GetAdminChannel(deadline_)} {}

ClientWrapper::~ClientWrapper() {
  try {
    admin_.RemoveExchange(GetExchange(), GetDeadline());
    admin_.RemoveQueue(GetQueue(), GetDeadline());
  } catch (const std::exception& ex) {
    LOG_ERROR() << "Failed to clean up after test execution, next tests might "
                   "fail randomly";
  }
}

urabbitmq::Client* ClientWrapper::operator->() const { return client_.get(); }

urabbitmq::Client& ClientWrapper::operator*() const {
  UASSERT(client_);
  return *client_;
}

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
  admin_.DeclareExchange(GetExchange(), urabbitmq::Exchange::Type::kFanOut, {},
                         GetDeadline());
  admin_.DeclareQueue(GetQueue(), {}, GetDeadline());
  admin_.BindQueue(GetExchange(), GetQueue(), GetRoutingKey(), GetDeadline());
}

namespace urabbitmq {

ClientSettings TestsHelper::CreateSettings() {
  urabbitmq::AuthSettings auth{};

  urabbitmq::EndpointInfo endpoint{};
  endpoint.port = GetRabbitMqPort();

  urabbitmq::RabbitEndpoints endpoints;
  endpoints.auth = std::move(auth);
  endpoints.endpoints = {std::move(endpoint)};

  urabbitmq::PoolSettings pool_settings{};
  pool_settings.min_pool_size = 3;
  pool_settings.max_pool_size = 5;

  urabbitmq::ClientSettings settings{};
  settings.pool_settings = pool_settings;
  settings.endpoints = std::move(endpoints);
  settings.use_secure_connection = false;

  return settings;
}

}  // namespace urabbitmq

USERVER_NAMESPACE_END
