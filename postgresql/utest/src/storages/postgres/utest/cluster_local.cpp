#include <userver/storages/postgres/utest/cluster_local.hpp>

#include <cstdlib>
#include <memory>
#include <vector>

#include <userver/engine/task/current_task.hpp>
#include <userver/error_injection/settings.hpp>
#include <userver/testsuite/postgres_control.hpp>
#include <userver/utils/statistics/metrics_storage.hpp>
#include <userver/utils/text_light.hpp>

#include <storages/postgres/default_command_controls.hpp>

USERVER_NAMESPACE_BEGIN

namespace storages::postgres::utest {

namespace {

constexpr const char* kPostgresDsn = "POSTGRES_TEST_DSN";

constexpr storages::postgres::CommandControl kTestCommandControl{
    std::chrono::seconds{2},
    std::chrono::milliseconds{500},
};

constexpr std::chrono::seconds kMaxTestWaitTime{20};

storages::postgres::DsnList GetDsnListFromEnv() {
    // NOLINTNEXTLINE(concurrency-mt-unsafe)
    auto* conn_list_env = std::getenv(kPostgresDsn);
    if (!conn_list_env) {
        return {storages::postgres::Dsn{"postgresql://"}};
    }

    storages::postgres::DsnList dsn_list;
    for (auto& conn : USERVER_NAMESPACE::utils::text::Split(conn_list_env, ";")) {
        dsn_list.insert(dsn_list.end(), storages::postgres::Dsn{std::move(conn)});
    }
    return dsn_list;
}

std::shared_ptr<storages::postgres::Cluster> CreateCluster(
    const storages::postgres::ClusterSettings& settings,
    testsuite::TestsuiteTasks& tasks
) {
    return std::make_shared<storages::postgres::Cluster>(
        GetDsnListFromEnv(),
        nullptr,
        engine::current_task::GetTaskProcessor(),
        settings,
        storages::postgres::DefaultCommandControls{kTestCommandControl, {}, {}},
        testsuite::PostgresControl{},
        error_injection::Settings{},
        tasks,
        dynamic_config::GetDefaultSource(),
        std::make_shared<USERVER_NAMESPACE::utils::statistics::MetricsStorage>(),
        0
    );
}

}  // namespace

storages::postgres::ClusterSettings MakeDefaultClusterSettings() {
    return storages::postgres::ClusterSettings{
        {},
        {kMaxTestWaitTime},
        {0, 1, 1},
        storages::postgres::ConnectionSettings{},
        storages::postgres::InitMode::kAsync,
        "",
        {},
        {},
    };
}

ClusterLocal::ClusterLocal()
    : ClusterLocal(MakeDefaultClusterSettings())
{}

ClusterLocal::ClusterLocal(const storages::postgres::ClusterSettings& settings)
    : settings_{settings},
      cluster_{CreateCluster(settings_, testsuite_tasks_)}
{}

ClusterLocal::~ClusterLocal() = default;

const std::shared_ptr<storages::postgres::Cluster>& ClusterLocal::GetCluster() const { return cluster_; }

void ClusterLocal::RenewCluster() {
    cluster_.reset();
    cluster_ = CreateCluster(settings_, testsuite_tasks_);
}

}  // namespace storages::postgres::utest

USERVER_NAMESPACE_END
