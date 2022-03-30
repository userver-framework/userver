#include "test_utils.hpp"

#include <userver/storages/clickhouse/settings.hpp>

#include <userver/components/component_config.hpp>
#include <userver/engine/task/task.hpp>
#include <userver/formats/yaml.hpp>

USERVER_NAMESPACE_BEGIN

namespace {
clients::dns::Resolver MakeDnsResolver() {
  return clients::dns::Resolver{engine::current_task::GetTaskProcessor(), {}};
}

components::ComponentConfig GetConfig() {
  USERVER_NAMESPACE::yaml_config::YamlConfig yaml_config{
      USERVER_NAMESPACE::formats::yaml::FromString(
          R"(
initial_pool_size: 1
max_pool_size: 10
queue_timeout: 1s
use_secure_connection: false)"),
      {}};
  return USERVER_NAMESPACE::components::ComponentConfig{std::move(yaml_config)};
}

storages::clickhouse::Cluster MakeCluster(clients::dns::Resolver& resolver) {
  storages::clickhouse::ClickhouseSettings settings;

  settings.auth_settings.user = "default";
  settings.auth_settings.password = "";
  settings.auth_settings.database = "default";
  settings.endpoints.emplace_back(
      storages::clickhouse::EndpointSettings{"localhost", 17123});

  return storages::clickhouse::Cluster{&resolver, settings, GetConfig()};
}
}  // namespace

ClusterWrapper::ClusterWrapper()
    : resolver_{MakeDnsResolver()}, cluster_{MakeCluster(resolver_)} {}

storages::clickhouse::Cluster* ClusterWrapper::operator->() {
  return &cluster_;
}

storages::clickhouse::Cluster& ClusterWrapper::operator*() { return cluster_; }

USERVER_NAMESPACE_END
