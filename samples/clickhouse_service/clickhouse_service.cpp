#include <string>
#include <vector>

#include <userver/utest/using_namespace_userver.hpp>

#include <userver/components/component.hpp>
#include <userver/components/minimal_server_component_list.hpp>

#include <userver/clients/dns/component.hpp>
#include <userver/server/handlers/http_handler_base.hpp>

#include <userver/storages/clickhouse/cluster.hpp>
#include <userver/storages/clickhouse/component.hpp>
#include <userver/storages/clickhouse/io/columns/string_column.hpp>
#include <userver/storages/clickhouse/query.hpp>

#include <userver/storages/secdist/component.hpp>
#include <userver/storages/secdist/provider_component.hpp>

#include <userver/utils/daemon_run.hpp>

namespace samples::clickhouse {

struct Result {
  std::vector<std::string> values;
};

class HandlerDb final : public server::handlers::HttpHandlerBase {
 public:
  static constexpr const char* kName = "handler-db";

  HandlerDb(const components::ComponentConfig& config,
            const components::ComponentContext& context);

  std::string HandleRequestThrow(
      const server::http::HttpRequest& request,
      server::request::RequestContext& context) const override;

 private:
  storages::clickhouse::ClusterPtr clickhouse_;
};

HandlerDb::HandlerDb(const components::ComponentConfig& config,
                     const components::ComponentContext& context)
    : server::handlers::HttpHandlerBase{config, context},
      clickhouse_{
          context.FindComponent<components::ClickHouse>("clickhouse-database")
              .GetCluster()} {}

std::string HandlerDb::HandleRequestThrow(
    const server::http::HttpRequest& request,
    server::request::RequestContext&) const {
  const auto& limit = request.GetArg("limit");
  // FP?: pfr magic
  // NOLINTNEXTLINE(clang-analyzer-core.uninitialized.UndefReturn)
  if (limit.empty()) {
    throw server::handlers::ClientError(
        server::handlers::ExternalBody{"No 'limit' query argument"});
  }

  const storages::clickhouse::Query query{
      "SELECT toString(c.number) FROM system.numbers c LIMIT toInt32({})"};
  const auto result = clickhouse_->Execute(query, limit).As<Result>();

  return fmt::to_string(fmt::join(result.values, ""));
}

}  // namespace samples::clickhouse

int main(int argc, char* argv[]) {
  const auto components_list =
      components::MinimalServerComponentList()
          .Append<samples::clickhouse::HandlerDb>()
          .Append<components::ClickHouse>("clickhouse-database")
          .Append<clients::dns::Component>()
          .Append<components::Secdist>()
          .Append<components::DefaultSecdistProvider>();

  return utils::DaemonMain(argc, argv, components_list);
}

USERVER_NAMESPACE_BEGIN

template <>
struct storages::clickhouse::io::CppToClickhouse<samples::clickhouse::Result> {
  using mapped_type = std::tuple<columns::StringColumn>;
};

USERVER_NAMESPACE_END
