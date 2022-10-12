#include <userver/clients/dns/component.hpp>
#include <userver/testsuite/testsuite_support.hpp>

#include <userver/utest/using_namespace_userver.hpp>

#include <userver/clients/http/component.hpp>
#include <userver/components/minimal_server_component_list.hpp>
#include <userver/server/handlers/http_handler_base.hpp>
#include <userver/server/handlers/tests_control.hpp>
#include <userver/testsuite/testpoint.hpp>
#include <userver/utils/daemon_run.hpp>

#include <userver/storages/postgres/cluster.hpp>
#include <userver/storages/postgres/component.hpp>

namespace chaos {

constexpr std::size_t kValuesCount = 10;

const storages::postgres::Query kInsertValue{
    "INSERT INTO key_value_table (key, value) "
    "VALUES ($1, $2) "
    "ON CONFLICT DO NOTHING",
    storages::postgres::Query::Name{"chaos_insert_value"},
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

  if (type == "fill") {
    storages::postgres::CommandControl cc{std::chrono::seconds(3),
                                          std::chrono::seconds(3)};
    TESTPOINT("before_trx_begin", {});
    auto transaction =
        pg_cluster_->Begin(storages::postgres::ClusterHostType::kMaster,
                           storages::postgres::TransactionOptions{}, cc);
    TESTPOINT("after_trx_begin", {});

    for (std::size_t i = 0; i < kValuesCount; ++i) {
      transaction.Execute(kInsertValue, fmt::format("key_{}", i),
                          fmt::format("value_{}", i));
    }

    TESTPOINT("before_trx_commit", {});
    transaction.Commit();
    TESTPOINT("after_trx_commit", {});
  } else {
    UINVARIANT(false, "Unknown chaos test request type");
  }

  return {};
}

}  // namespace chaos

int main(int argc, char* argv[]) {
  const auto component_list =
      components::MinimalServerComponentList()
          .Append<chaos::PostgresHandler>()
          .Append<components::HttpClient>()
          .Append<components::Postgres>("key-value-database")
          .Append<components::TestsuiteSupport>()
          .Append<server::handlers::TestsControl>()
          .Append<clients::dns::Component>();
  return utils::DaemonMain(argc, argv, component_list);
}
