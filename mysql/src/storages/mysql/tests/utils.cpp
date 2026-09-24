#include <userver/storages/mysql/tests/utils.hpp>

#include <cstdlib>

#include <chrono>
#include <thread>

#include <fmt/format.h>

#include <userver/components/component_config.hpp>
#include <userver/engine/deadline.hpp>
#include <userver/engine/task/current_task.hpp>
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

settings::ConnectionSettings MakeTestConnectionSettings() {
    settings::ConnectionSettings connection_settings{};
    connection_settings.statements_cache_size = 20;
    connection_settings.use_secure_connection = false;
    connection_settings.use_compression = false;
    connection_settings.ip_mode = settings::IpMode::kIpV4;
    return connection_settings;
}

void DoCreateTestDatabase(clients::dns::Resolver& resolver, std::uint32_t port) {
    // TODO provide an in-framework API for MySQL database creation.
    constexpr auto kRetryCount = 30;
    constexpr auto kRetryDelay = std::chrono::milliseconds{200};
    constexpr auto kConnectTimeout = std::chrono::seconds{5};

    settings::EndpointInfo endpoint{"127.0.0.1", port};
    settings::AuthSettings auth{};
    auth.user = "root";
    auth.password = decltype(auth.password){""};
    auth.database.clear();

    const auto connection_settings = MakeTestConnectionSettings();
    const auto deadline = engine::Deadline::FromDuration(kConnectTimeout);

    for (int attempt = 0; attempt < kRetryCount; ++attempt) {
        try {
            impl::Connection connection(resolver, endpoint, auth, connection_settings, deadline);
            connection.ExecuteQuery("CREATE DATABASE IF NOT EXISTS userver_mysql_test", deadline);
            return;
        } catch (const std::exception&) {
        }

        std::this_thread::sleep_for(kRetryDelay);
    }

    throw std::runtime_error(fmt::format(
        "Failed to create test database on port {} after {} attempts. "
        "Ensure MySQL is running and accepts connections from 127.0.0.1",
        port,
        kRetryCount
    ));
}

std::uint32_t GetTestDatabasePort() {
    // NOLINTNEXTLINE(concurrency-mt-unsafe)
    if (const char* port_string = std::getenv("TESTSUITE_MYSQL_PORT")) {
        return utils::FromString<std::uint32_t>(port_string);
    }
    return kDefaultTestsuiteMysqlPort;
}

void CreateTestDatabase(clients::dns::Resolver& resolver) {
    static bool created = false;
    if (!created) {
        DoCreateTestDatabase(resolver, GetTestDatabasePort());
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
    CreateTestDatabase(resolver);

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
        {}
    }};

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
      deadline_{engine::Deadline::FromDuration(std::chrono::seconds{20})}
{}

ClusterWrapper::~ClusterWrapper() = default;

storages::mysql::Cluster& ClusterWrapper::operator*() const { return *cluster_; }

storages::mysql::Cluster* ClusterWrapper::operator->() const { return cluster_.get(); }

engine::Deadline ClusterWrapper::GetDeadline() const { return deadline_; }

TmpTable::TmpTable(std::string_view definition)
    : owned_cluster_{std::in_place},
      cluster_{*owned_cluster_},
      table_name_{GenerateTableName()}
{
    CreateTable(definition);
}

TmpTable::TmpTable(ClusterWrapper& cluster, std::string_view definition)
    : cluster_{cluster},
      table_name_{GenerateTableName()}
{
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
