#include <userver/storages/mysql/tests/utils.hpp>

#include <stdlib.h>

#include <cstdlib>

#include <fmt/format.h>

#include <userver/components/component_config.hpp>
#include <userver/engine/subprocess/process_starter.hpp>
#include <userver/engine/task/task.hpp>
#include <userver/formats/json.hpp>
#include <userver/formats/yaml.hpp>
#include <userver/fs/blocking/read.hpp>
#include <userver/fs/blocking/temp_file.hpp>
#include <userver/utils/from_string.hpp>

#include <storages/mysql/impl/connection.hpp>
#include <storages/mysql/settings/settings.hpp>

USERVER_NAMESPACE_BEGIN

namespace storages::mysql::tests {

namespace {

constexpr const char* kTestsuiteMysqlPort = "TESTSUITE_MYSQL_PORT";
constexpr std::uint32_t kDefaultTestsuiteMysqlPort = 13307;

void DoCreateTestDatabase(std::uint32_t port) {
    // TODO provide an in-framework API for MySQL database creation.

    // engine::subprocess::ProcessStarter is not available here, because
    // the default ev loop is disabled due to death-tests.
    // Also, we want to avoid having a dependency on Boost.Process.

    // There definitely won't be any shell injection here, trust me.
    // Also, this hack is run in tests only.
    // NOLINTNEXTLINE(cert-env33-c,concurrency-mt-unsafe)
    const auto status = std::system(
        fmt::format(R"(mysql -u root -h 127.0.0.1 -P {} -e "CREATE DATABASE IF NOT EXISTS userver_mysql_test")", port)
            .c_str()
    );

    if (!WIFEXITED(status) || WEXITSTATUS(status) != 0) {
        throw std::runtime_error("Failed to create test database");
    }
}

std::uint32_t GetTestDatabasePort() {
    // NOLINTNEXTLINE(concurrency-mt-unsafe)
    if (const char* port_string = std::getenv("TESTSUITE_MYSQL_PORT")) {
        return utils::FromString<std::uint32_t>(port_string);
    }
    return kDefaultTestsuiteMysqlPort;
}

void CreateTestDatabase() {
    static bool created = false;
    if (!created) {
        DoCreateTestDatabase(GetTestDatabasePort());
        created = true;
    }
}

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

std::shared_ptr<Cluster> CreateCluster(clients::dns::Resolver& resolver) {
    CreateTestDatabase();

    formats::json::ValueBuilder secdist_json_builder = formats::json::FromString(R"(
    {
      "hosts": ["localhost"],
      "database": "userver_mysql_test",
      "user": "root",
      "password": ""
    }
  )");
    secdist_json_builder["port"] = GetMysqlPort();
    const auto secdist_json = secdist_json_builder.ExtractValue();
    const auto settings = secdist_json.As<settings::MysqlSettings>();

    const components::ComponentConfig config{yaml_config::YamlConfig{
        formats::yaml::FromString(R"(
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
    return mysql_port_env ? utils::FromString<std::uint32_t>(mysql_port_env) : kDefaultTestsuiteMysqlPort;
}

std::chrono::system_clock::time_point ToMariaDBPrecision(std::chrono::system_clock::time_point tp) {
    return std::chrono::time_point_cast<std::chrono::microseconds>(tp);
}

ClusterWrapper::ClusterWrapper()
    : resolver_(engine::current_task::GetTaskProcessor(), {}),
      cluster_{CreateCluster(resolver_)},
      deadline_{engine::Deadline::FromDuration(std::chrono::seconds{20})} {}

ClusterWrapper::~ClusterWrapper() = default;

storages::mysql::Cluster& ClusterWrapper::operator*() const { return *cluster_; }

storages::mysql::Cluster* ClusterWrapper::operator->() const { return cluster_.get(); }

engine::Deadline ClusterWrapper::GetDeadline() const { return deadline_; }

TmpTable::TmpTable(std::string_view definition)
    : owned_cluster_{std::in_place}, cluster_{*owned_cluster_}, table_name_{GenerateTableName()} {
    CreateTable(definition);
}

TmpTable::TmpTable(ClusterWrapper& cluster, std::string_view definition)
    : cluster_{cluster}, table_name_{GenerateTableName()} {
    CreateTable(definition);
}

// We don't drop table here because it seems to be very slow
TmpTable::~TmpTable() = default;

ClusterWrapper& TmpTable::GetCluster() const { return cluster_; }

Transaction TmpTable::Begin() { return cluster_->Begin(ClusterHostType::kPrimary); }

engine::Deadline TmpTable::GetDeadline() const { return cluster_.GetDeadline(); }

void TmpTable::CreateTable(std::string_view definition) {
    const auto create_table_query = fmt::format(kCreateTableQueryTemplate, table_name_, definition);

    cluster_->ExecuteCommand(ClusterHostType::kPrimary, create_table_query);
}

}  // namespace storages::mysql::tests

USERVER_NAMESPACE_END
