#include <userver/clients/dns/component.hpp>
#include <userver/testsuite/testsuite_support.hpp>

#include <userver/utest/using_namespace_userver.hpp>

#include <userver/clients/http/component.hpp>
#include <userver/components/minimal_server_component_list.hpp>
#include <userver/dynamic_config/client/component.hpp>
#include <userver/dynamic_config/updater/component.hpp>
#include <userver/server/handlers/http_handler_base.hpp>
#include <userver/server/handlers/tests_control.hpp>
#include <userver/testsuite/testpoint.hpp>
#include <userver/utils/daemon_run.hpp>

#include <userver/storages/postgres/cluster.hpp>
#include <userver/storages/postgres/component.hpp>

namespace chaos {

constexpr std::string_view kSelectSmallTimeout = "select-small-timeout";
constexpr std::string_view kPortalSmallTimeout = "portal-small-timeout";

const storages::postgres::Query kSelectMany{
    "SELECT generate_series(1, 100)",
    storages::postgres::Query::Name{"chaos_select_many"},
};

constexpr int kPortalChunkSize = 4;

const storages::postgres::Query kPortalQuery{
    "SELECT generate_series(1, 10)",
    storages::postgres::Query::Name{"portal_query"},
};

class PostgresHandler final : public server::handlers::HttpHandlerBase {
 public:
  static constexpr std::string_view kName = "handler-chaos-postgres";

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

  if (type == "select" || type == kSelectSmallTimeout) {
    const std::chrono::seconds timeout{type == kSelectSmallTimeout ? 1 : 30};

    storages::postgres::CommandControl cc{timeout, timeout};
    TESTPOINT("before_trx_begin", {});
    auto transaction =
        pg_cluster_->Begin(storages::postgres::ClusterHostType::kMaster,
                           storages::postgres::TransactionOptions{}, cc);
    TESTPOINT("after_trx_begin", {});

    // Disk on CI could be overloaded, so we use a lightweight query.
    //
    // pgsql in testsuite works with sockets synchronously.
    // We use non writing queries to avoid following deadlocks:
    // 1) python test finished, connection is broken, postgres table is
    //    write locked
    // 2) pgsql starts the tables cleanup and hangs on a poll (because of a
    //    write lock from 1)
    // 3) C++ driver does an async cleanup and closes the connection
    // 4) chaos proxy does not get a time slice (because of 2), the socket is
    //    not closed by postgres, 2) hangs
    transaction.Execute(cc, kSelectMany);

    TESTPOINT("before_trx_commit", {});
    transaction.Commit();
    TESTPOINT("after_trx_commit", {});
    return {};
  } else if (type == "portal" || type == kPortalSmallTimeout) {
    const std::chrono::seconds timeout{type == kPortalSmallTimeout ? 3 : 25};

    storages::postgres::CommandControl cc{timeout, timeout};
    auto transaction =
        pg_cluster_->Begin(storages::postgres::ClusterHostType::kMaster,
                           storages::postgres::TransactionOptions{}, cc);

    auto portal = transaction.MakePortal(cc, kPortalQuery);
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
    UINVARIANT(false,
               fmt::format("Unknown chaos test request type '{}'", type));
  }

  return {};
}

}  // namespace chaos

int main(int argc, char* argv[]) {
  const auto component_list =
      components::MinimalServerComponentList()
          .Append<components::DynamicConfigClient>()
          .Append<components::DynamicConfigClientUpdater>()
          .Append<chaos::PostgresHandler>()
          .Append<components::HttpClient>()
          .Append<components::Postgres>("key-value-database")
          .Append<components::TestsuiteSupport>()
          .Append<server::handlers::TestsControl>()
          .Append<clients::dns::Component>();
  return utils::DaemonMain(argc, argv, component_list);
}
