#include <userver/clients/dns/component.hpp>
#include <userver/testsuite/testsuite_support.hpp>

#include <userver/utest/using_namespace_userver.hpp>

#include <userver/clients/http/component.hpp>
#include <userver/components/minimal_server_component_list.hpp>
#include <userver/server/handlers/http_handler_base.hpp>
#include <userver/server/handlers/server_monitor.hpp>
#include <userver/server/handlers/tests_control.hpp>
#include <userver/testsuite/testpoint.hpp>
#include <userver/utils/daemon_run.hpp>

#include <userver/storages/postgres/cluster.hpp>
#include <userver/storages/postgres/component.hpp>
#include <userver/storages/postgres/dist_lock_component_base.hpp>

#include <userver/cache/base_postgres_cache.hpp>

namespace pg::metrics {

struct KeyValue {
  std::string key;
  std::string value;
};

struct KeyValueCachePolicy {
  static constexpr std::string_view kName = "key-value-pg-cache";

  using ValueType = KeyValue;
  static constexpr auto kKeyMember = &KeyValue::key;
  static constexpr const char* kQuery =
      "SELECT key, value FROM key_value_table";
  static constexpr const char* kUpdatedField = "updated";
  using UpdatedFieldType = storages::postgres::TimePointTz;
};

using KeyValueCache = components::PostgreCache<KeyValueCachePolicy>;

const storages::postgres::Query kInsertValue{
    "INSERT INTO key_value_table (key, value) "
    "VALUES ($1, $2) "
    "ON CONFLICT DO NOTHING",
    storages::postgres::Query::Name{"metrics_insert_value"},
};

constexpr int kPortalChunkSize = 4;

const storages::postgres::Query kPortalQuery{
    "SELECT generate_series(1, 10)",
    storages::postgres::Query::Name{"portal_query"},
};

class PostgresHandler final : public server::handlers::HttpHandlerBase {
 public:
  static constexpr std::string_view kName = "handler-metrics-postgres";

  PostgresHandler(const components::ComponentConfig& config,
                  const components::ComponentContext& context);

  std::string HandleRequestThrow(
      const server::http::HttpRequest& request,
      server::request::RequestContext&) const override;

 private:
  storages::postgres::ClusterPtr pg_cluster_;
};

PostgresHandler::PostgresHandler(const components::ComponentConfig& config,
                                 const components::ComponentContext& context)
    : HttpHandlerBase(config, context),
      pg_cluster_(
          context.FindComponent<components::Postgres>("key-value-database")
              .GetCluster()) {}

std::string PostgresHandler::HandleRequestThrow(
    const server::http::HttpRequest& request,
    server::request::RequestContext&) const {
  const auto& type = request.GetArg("type");
  if (type.empty()) {
    throw server::handlers::ClientError(
        server::handlers::ExternalBody{"No 'type' query argument"});
  }

  if (type == "fill") {
    storages::postgres::CommandControl cc{std::chrono::seconds(1),
                                          std::chrono::seconds(1)};
    auto transaction =
        pg_cluster_->Begin(storages::postgres::ClusterHostType::kMaster,
                           storages::postgres::TransactionOptions{}, cc);

    transaction.Execute(kInsertValue, "key_0", "value_0");
    transaction.Commit();
    return {};
  } else if (type == "portal") {
    storages::postgres::CommandControl cc{std::chrono::seconds(3),
                                          std::chrono::seconds(3)};
    auto transaction =
        pg_cluster_->Begin(storages::postgres::ClusterHostType::kMaster,
                           storages::postgres::TransactionOptions{}, cc);

    auto portal = transaction.MakePortal(kPortalQuery);
    TESTPOINT("after_make_portal", {});

    std::vector<int> result;
    while (portal) {
      auto res = portal.Fetch(kPortalChunkSize);
      TESTPOINT("after_fetch", {});

      auto vec = res.AsContainer<std::vector<int>>();
      result.insert(result.end(), vec.begin(), vec.end());
    }

    transaction.Commit();
    return fmt::format("[{}]", fmt::join(result, ", "));
  } else {
    UINVARIANT(false, "Unknown metrics test request type");
  }

  return {};
}

class DistlockMetrics final : public storages::postgres::DistLockComponentBase {
 public:
  static constexpr std::string_view kName = "component-distlock-metrics";

  DistlockMetrics(const components::ComponentConfig& config,
                  const components::ComponentContext& context)
      : storages::postgres::DistLockComponentBase(config, context) {
    AutostartDistLock();
  }

  ~DistlockMetrics() override { StopDistLock(); }

  void DoWork() override {
    // noop
  }
};

}  // namespace pg::metrics

int main(int argc, char* argv[]) {
  const auto component_list =
      components::MinimalServerComponentList()
          .Append<server::handlers::ServerMonitor>()
          .Append<pg::metrics::PostgresHandler>()
          .Append<pg::metrics::KeyValueCache>()
          .Append<pg::metrics::DistlockMetrics>()
          .Append<components::HttpClient>()
          .Append<components::Postgres>("key-value-database")
          .Append<components::TestsuiteSupport>()
          .Append<server::handlers::TestsControl>()
          .Append<clients::dns::Component>();
  return utils::DaemonMain(argc, argv, component_list);
}
