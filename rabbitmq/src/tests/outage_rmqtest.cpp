#include "utils_rmqtest.hpp"

USERVER_NAMESPACE_BEGIN

namespace {

clients::dns::Resolver CreateResolver() {
  return clients::dns::Resolver{engine::current_task::GetTaskProcessor(), {}};
}

urabbitmq::ClientSettings GetSettings(
    std::vector<urabbitmq::EndpointInfo> endpoints) {
  auto settings = urabbitmq::TestsHelper::CreateSettings();
  settings.use_secure_connection = false;
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

  // just some small timeout, it should fail anyway
  const std::chrono::milliseconds timeout{20};
  // But any operation expectedly throws
  EXPECT_ANY_THROW(
      client->GetAdminChannel(engine::Deadline::FromDuration(timeout)));
  EXPECT_ANY_THROW(client->GetChannel(engine::Deadline::FromDuration(timeout)));
  EXPECT_ANY_THROW(
      client->GetReliableChannel(engine::Deadline::FromDuration(timeout)));
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
  const auto deadline = engine::Deadline::FromDuration(utest::kMaxTestWaitTime);
  for (size_t i = 0; i < operations_count; ++i) {
    try {
      auto channel = client->GetChannel(deadline);
      ++success;
    } catch (const std::exception&) {
    }
  }

  // And every operation goes through alive host
  EXPECT_EQ(success, operations_count);
}

USERVER_NAMESPACE_END
