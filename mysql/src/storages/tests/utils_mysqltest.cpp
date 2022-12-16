#include "utils_mysqltest.hpp"

#include <fmt/format.h>

#include <userver/components/component_config.hpp>
#include <userver/engine/task/task.hpp>
#include <userver/formats/json.hpp>
#include <userver/formats/yaml.hpp>
#include <userver/utils/from_string.hpp>
#include <userver/utils/uuid4.hpp>

#include <storages/mysql/impl/mysql_connection.hpp>
#include <storages/mysql/settings/settings.hpp>

USERVER_NAMESPACE_BEGIN

namespace storages::mysql::tests {

namespace {

constexpr const char* kTestsuiteMysqlPort = "TESTSUITE_MYSQL_PORT";
constexpr std::uint32_t kDefaultTestsuiteMysqlPort = 3306;  // 13307;

std::string GenerateTableName() {
  auto uuid = utils::generators::GenerateUuid();

  std::string name{"tmp_"};
  name.reserve(4 + uuid.size());
  for (const auto c : uuid) {
    if (c != '-') {
      name.push_back(c);
    }
  }

  return name;
}

void CreateDatabase(clients::dns::Resolver& resolver,
                    const settings::EndpointInfo& endpoint_info,
                    const std::string& dbname) {
  settings::AuthSettings auth_settings{};
  auth_settings.user = "root";
  auth_settings.database = "";

  const auto deadline = engine::Deadline::FromDuration(std::chrono::seconds{1});
  impl::MySQLConnection{resolver, endpoint_info, auth_settings, deadline}
      .ExecutePlain(fmt::format("CREATE DATABASE IF NOT EXISTS {}", dbname),
                    deadline);
}

std::shared_ptr<Cluster> CreateCluster(clients::dns::Resolver& resolver) {
  const settings::MysqlSettings settings{
      formats::json::FromString(fmt::format(R"(
  {{
    "port": {},
    "hosts": ["localhost"],
    "database": "userver_mysql_test",
    "user": "root",
    "password": ""
  }})",
                                            GetMysqlPort()))};

  const components::ComponentConfig config{
      yaml_config::YamlConfig{formats::yaml::FromString(R"(
    initial_pool_size: 1
    max_pool_size: 5
  )"),
                              {}}};

  // CreateDatabase(resolver, settings.endpoints.front(),
  // settings.auth.database);

  return std::make_shared<Cluster>(resolver, settings, config);
}

}  // namespace

std::uint32_t GetMysqlPort() {
  // NOLINTNEXTLINE(concurrency-mt-unsafe)
  const auto* mysql_port_env = std::getenv(kTestsuiteMysqlPort);
  return mysql_port_env ? utils::FromString<std::uint32_t>(mysql_port_env)
                        : kDefaultTestsuiteMysqlPort;
}

std::chrono::system_clock::time_point ToMariaDBPrecision(
    std::chrono::system_clock::time_point tp) {
  return std::chrono::time_point_cast<std::chrono::microseconds>(tp);
}

std::string TestsHelper::EscapeString(const Cluster& cluster,
                                      std::string_view source) {
  return cluster.EscapeString(source);
}

ClusterWrapper::ClusterWrapper()
    : resolver_(engine::current_task::GetTaskProcessor(), {}),
      cluster_{CreateCluster(resolver_)},
      // TODO : return to normal
      deadline_{engine::Deadline::FromDuration(std::chrono::seconds{500})} {}
// deadline_{engine::Deadline::FromDuration(utest::kMaxTestWaitTime)} {

ClusterWrapper::~ClusterWrapper() = default;

storages::mysql::Cluster& ClusterWrapper::operator*() const {
  return *cluster_;
}

storages::mysql::Cluster* ClusterWrapper::operator->() const {
  return cluster_.get();
}

engine::Deadline ClusterWrapper::GetDeadline() const { return deadline_; }

TmpTable::TmpTable(std::string_view definition)
    : owned_cluster_{std::in_place},
      cluster_{*owned_cluster_},
      table_name_{GenerateTableName()} {
  CreateTable(definition);
}

TmpTable::TmpTable(ClusterWrapper& cluster, std::string_view definition)
    : cluster_{cluster}, table_name_{GenerateTableName()} {
  CreateTable(definition);
}

// We don't drop table here because it seems to be very slow
TmpTable::~TmpTable() = default;

ClusterWrapper& TmpTable::GetCluster() const { return cluster_; }

Transaction TmpTable::Begin() {
  return cluster_->Begin(ClusterHostType::kMaster, cluster_.GetDeadline());
}

engine::Deadline TmpTable::GetDeadline() const {
  return cluster_.GetDeadline();
}

void TmpTable::CreateTable(std::string_view definition) {
  const auto create_table_query =
      fmt::format(kCreateTableQueryTemplate, table_name_, definition);

  cluster_->ExecuteCommand(ClusterHostType::kMaster, cluster_.GetDeadline(),
                           create_table_query);
}

}  // namespace storages::mysql::tests

USERVER_NAMESPACE_END
