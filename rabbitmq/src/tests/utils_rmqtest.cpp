#include "utils_rmqtest.hpp"

#include <userver/utils/uuid4.hpp>

USERVER_NAMESPACE_BEGIN

namespace {

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
  // endpoint.port = 5673;

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
      routing_key_{utils::generators::GenerateUuid()} {}

ClientWrapper::~ClientWrapper() {
  auto channel = client_->GetAdminChannel();
  channel.RemoveExchange(GetExchange());
  channel.RemoveQueue(GetQueue());
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

void ClientWrapper::SetupRmqEntities() const {
  auto channel = client_->GetAdminChannel();
  channel.DeclareExchange(GetExchange(), urabbitmq::ExchangeType::kFanOut);
  channel.DeclareQueue(GetQueue());
  channel.BindQueue(GetExchange(), GetQueue(), GetRoutingKey());
}

USERVER_NAMESPACE_END
