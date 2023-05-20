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
#include "userver/clients/http/component.hpp"
#include "userver/server/handlers/server_monitor.hpp"
#include "userver/server/handlers/tests_control.hpp"
#include "userver/storages/clickhouse/io/columns/datetime_column.hpp"
#include "userver/testsuite/testsuite_support.hpp"

namespace clickhouse::metrics {

struct KeyValueRow {
  std::string key;
  std::string value;
  std::chrono::system_clock::time_point updated;
};

struct KeyValue {
  std::vector<std::string> keys;
  std::vector<std::string> values;
  std::vector<std::chrono::system_clock::time_point> updates;
};

struct Result {
  std::vector<KeyValue> values;
};

class HandlerMetricsClickhouse final
    : public server::handlers::HttpHandlerBase {
 public:
  static constexpr const char* kName = "handler-metrics-clickhouse";

  HandlerMetricsClickhouse(const components::ComponentConfig& config,
                           const components::ComponentContext& context);

  std::string HandleRequestThrow(
      const server::http::HttpRequest& request,
      server::request::RequestContext& context) const override;

 private:
  storages::clickhouse::ClusterPtr clickhouse_;
};

HandlerMetricsClickhouse::HandlerMetricsClickhouse(
    const components::ComponentConfig& config,
    const components::ComponentContext& context)
    : server::handlers::HttpHandlerBase{config, context},
      clickhouse_{
          context.FindComponent<components::ClickHouse>("clickhouse-database")
              .GetCluster()} {}

std::string HandlerMetricsClickhouse::HandleRequestThrow(
    const server::http::HttpRequest& request,
    server::request::RequestContext&) const {
  auto key = request.GetArg("key");
  auto value = request.GetArg("value");
  if (key.empty()) {
    request.GetHttpResponse().SetStatus(server::http::HttpStatus::kBadRequest);
  }

  if (request.GetMethod() == server::http::HttpMethod::kPost) {
    if (value.empty()) {
      request.GetHttpResponse().SetStatus(
          server::http::HttpStatus::kBadRequest);
      return "Bad Request";
    }
    KeyValue data;
    data.keys.push_back(key);
    data.values.push_back(value);
    data.updates.push_back(utils::datetime::Now());
    clickhouse_->Insert("key_value_table", {"key", "value", "updated"}, data);
  } else if (request.GetMethod() == server::http::HttpMethod::kDelete) {
    const storages::clickhouse::Query query{
        "ALTER TABLE key_value_table DELETE WHERE key={}"};
    clickhouse_->Execute(query, key);
    return fmt::format("Deleted by key: {}", key);
  }
  const storages::clickhouse::Query query{
      "SELECT key, value, updated FROM key_value_table WHERE key={}"};

  const auto result = clickhouse_->Execute(query, key).As<KeyValue>();

  std::string ret{};
  if (!result.keys.empty()) {
    ret += fmt::format("{}: {}", result.keys.back(), result.values.back());
  }
  return ret;
}

}  // namespace clickhouse::metrics

int main(int argc, char* argv[]) {
  const auto components_list =
      components::MinimalServerComponentList()
          .Append<server::handlers::ServerMonitor>()
          .Append<clickhouse::metrics::HandlerMetricsClickhouse>()
          .Append<components::ClickHouse>("clickhouse-database")
          .Append<components::HttpClient>()
          .Append<components::TestsuiteSupport>()
          .Append<server::handlers::TestsControl>()
          .Append<clients::dns::Component>()
          .Append<components::Secdist>()
          .Append<components::DefaultSecdistProvider>();

  return utils::DaemonMain(argc, argv, components_list);
}

USERVER_NAMESPACE_BEGIN
namespace storages::clickhouse::io {

template <>
struct CppToClickhouse<::clickhouse::metrics::KeyValue> final {
  using mapped_type = std::tuple<columns::StringColumn, columns::StringColumn,
                                 columns::DateTimeColumn>;
};

template <>
struct CppToClickhouse<::clickhouse::metrics::KeyValueRow> final {
  using mapped_type = std::tuple<columns::StringColumn, columns::StringColumn,
                                 columns::DateTimeColumn>;
};

template <>
struct CppToClickhouse<::clickhouse::metrics::Result> final {
  using mapped_type = std::tuple<::clickhouse::metrics::KeyValue>;
};

USERVER_NAMESPACE_END

}  // namespace storages::clickhouse::io
