#include "utils_rmqtest.hpp"

USERVER_NAMESPACE_BEGIN

namespace {

clients::dns::Resolver CreateResolver() {
  return clients::dns::Resolver{engine::current_task::GetTaskProcessor(), {}};
}

urabbitmq::ClientSettings GetSettings(
    std::vector<urabbitmq::EndpointInfo> endpoints) {
  urabbitmq::ClientSettings settings{};
  settings.secure = false;
  settings.endpoints.endpoints = std::move(endpoints);

  return settings;
}

}  // namespace

UTEST(ClusterOutage, AllNodesDead) {
  auto resolver = CreateResolver();
  const auto settings =
      GetSettings({{"unresolved", 5672}, {"another_unresolved", 5672}});

  // Client is created successfully
  auto client = urabbitmq::Client::Create(resolver, settings);

  // But any operation expectedly throws
  EXPECT_ANY_THROW(client->GetAdminChannel());
  EXPECT_ANY_THROW(client->GetChannel());
  EXPECT_ANY_THROW(client->GetReliableChannel());
}

UTEST(ClusterOutage, HasAliveNodes) {
  auto resolver = CreateResolver();
  const auto settings = GetSettings({{"unresolved", 5672},
                                     {"localhost", GetRabbitMqPort()},
                                     {"another_unresolved", 5672}});

  // Client is created successfully
  auto client = urabbitmq::Client::Create(resolver, settings);

  size_t success = 0;
  const auto operations_count = 100;
  for (size_t i = 0; i < operations_count; ++i) {
    try {
      auto channel = client->GetChannel();
      ++success;
    } catch (const std::exception&) {
    }
  }

  // And every operation goes through alive host
  EXPECT_EQ(success, operations_count);
}

USERVER_NAMESPACE_END
