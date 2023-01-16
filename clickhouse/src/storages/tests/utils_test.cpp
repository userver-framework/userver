#include "utils_test.hpp"

#include <cstdlib>
#include <string>

#include <userver/components/component_config.hpp>
#include <userver/engine/task/task.hpp>
#include <userver/formats/yaml.hpp>
#include <userver/utils/from_string.hpp>

#include <storages/clickhouse/impl/settings.hpp>

USERVER_NAMESPACE_BEGIN

namespace {

constexpr const char* kTestsuiteClickhouseTcpPort =
    "TESTSUITE_CLICKHOUSE_SERVER_TCP_PORT";
constexpr std::uint32_t kDefaultClickhousePort = 17123;

clients::dns::Resolver MakeDnsResolver() {
  return clients::dns::Resolver{engine::current_task::GetTaskProcessor(), {}};
}

components::ComponentConfig GetConfig(bool use_compression) {
  USERVER_NAMESPACE::formats::yaml::ValueBuilder config_builder{
      USERVER_NAMESPACE::formats::yaml::FromString(
          R"(
initial_pool_size: 1
max_pool_size: 10
queue_timeout: 1s
use_secure_connection: false
use_compression: false)")};
  if (use_compression) {
    config_builder["compression"] = "lz4";
  }

  USERVER_NAMESPACE::yaml_config::YamlConfig yaml_config{
      config_builder.ExtractValue(), {}};
  return USERVER_NAMESPACE::components::ComponentConfig{std::move(yaml_config)};
}

storages::clickhouse::impl::AuthSettings GetAuthSettings() {
  storages::clickhouse::impl::AuthSettings settings;
  settings.user = "default";
  settings.password = "";
  settings.database = "default";

  return settings;
}

storages::clickhouse::Cluster MakeCluster(
    clients::dns::Resolver& resolver, bool use_compression,
    const std::vector<storages::clickhouse::impl::EndpointSettings>&
        endpoints) {
  storages::clickhouse::impl::ClickhouseSettings settings;
  settings.auth_settings = GetAuthSettings();
  settings.endpoints = endpoints;

  return storages::clickhouse::Cluster{resolver, settings,
                                       GetConfig(use_compression)};
}

}  // namespace

uint32_t GetClickhousePort() {
  // NOLINTNEXTLINE(concurrency-mt-unsafe)
  const auto* clickhouse_port_env = std::getenv(kTestsuiteClickhouseTcpPort);
  return clickhouse_port_env
             ? utils::FromString<std::uint32_t>(clickhouse_port_env)
             : kDefaultClickhousePort;
}

ClusterWrapper::ClusterWrapper(
    bool use_compression,
    const std::vector<storages::clickhouse::impl::EndpointSettings>& endpoints)
    : resolver_{MakeDnsResolver()},
      cluster_{MakeCluster(resolver_, use_compression, endpoints)} {
  stats_holder_ = statistics_storage_.RegisterWriter(
      "clickhouse", [this](utils::statistics::Writer& writer) {
        cluster_.WriteStatistics(writer);
      });
}

storages::clickhouse::Cluster* ClusterWrapper::operator->() {
  return &cluster_;
}

storages::clickhouse::Cluster& ClusterWrapper::operator*() { return cluster_; }

utils::statistics::Snapshot ClusterWrapper::GetStatistics(
    std::string prefix, std::vector<utils::statistics::Label> require_labels) {
  return utils::statistics::Snapshot{statistics_storage_, std::move(prefix),
                                     std::move(require_labels)};
}
PoolWrapper::PoolWrapper()
    : resolver_{MakeDnsResolver()},
      pool_{std::make_shared<storages::clickhouse::impl::PoolImpl>(
          resolver_, storages::clickhouse::impl::PoolSettings{
                         GetConfig(false),
                         {"localhost", GetClickhousePort()},
                         GetAuthSettings()})} {}

storages::clickhouse::impl::PoolImpl* PoolWrapper::operator->() {
  return &*pool_;
}

storages::clickhouse::impl::PoolImpl& PoolWrapper::operator*() {
  return *pool_;
}

namespace storages::clickhouse::io {

std::optional<std::string> IteratorsTester::GetCurrentValue(
    columns::ColumnIterator<columns::StringColumn>& iterator) {
  return iterator.data_.current_value_;
}

}  // namespace storages::clickhouse::io

USERVER_NAMESPACE_END
