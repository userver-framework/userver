#include <userver/utest/using_namespace_userver.hpp>

#include <string_view>
#include <vector>

#include <userver/clients/dns/component.hpp>
#include <userver/components/minimal_server_component_list.hpp>
#include <userver/server/handlers/http_handler_base.hpp>
#include <userver/storages/clickhouse.hpp>
#include <userver/storages/secdist/component.hpp>
#include <userver/storages/secdist/provider_component.hpp>
#include <userver/utils/daemon_run.hpp>

namespace chaos {

struct KeyValueRow final {
  std::string key;
  std::string value;
};

class KeyValue final : public server::handlers::HttpHandlerBase {
 public:
  static constexpr std::string_view kName{"handler-chaos"};

  KeyValue(const components::ComponentConfig& config,
           const components::ComponentContext& context);

  std::string HandleRequestThrow(
      const server::http::HttpRequest& request,
      server::request::RequestContext&) const override;

 private:
  static constexpr std::chrono::milliseconds kDefaultTimeout{2000};

  std::string GetValue(std::string_view key,
                       const server::http::HttpRequest& request) const;
  std::string PostValue(std::string_view key,
                        const server::http::HttpRequest& request) const;
  std::string DeleteValue(std::string_view key) const;

  storages::clickhouse::ClusterPtr clickhouse_client_;
  storages::clickhouse::CommandControl cc_{kDefaultTimeout};
};

KeyValue::KeyValue(const components::ComponentConfig& config,
                   const components::ComponentContext& context)
    : server::handlers::HttpHandlerBase{config, context},
      clickhouse_client_{
          context.FindComponent<components::ClickHouse>("clickhouse-database")
              .GetCluster()} {}

std::string KeyValue::HandleRequestThrow(
    const server::http::HttpRequest& request,
    server::request::RequestContext&) const {
  const auto& key = request.GetArg("key");
  if (key.empty()) {
    throw server::handlers::ClientError{
        server::handlers::ExternalBody{"No 'key' query argument"}};
  }

  switch (request.GetMethod()) {
    case server::http::HttpMethod::kGet:
      return GetValue(key, request);
    case server::http::HttpMethod::kPost:
      return PostValue(key, request);
    case server::http::HttpMethod::kDelete:
      return DeleteValue(key);
    default:
      throw server::handlers::ClientError{server::handlers::ExternalBody{
          fmt::format("Unsupported method {}", request.GetMethod())}};
  }
}

std::string KeyValue::GetValue(std::string_view key,
                               const server::http::HttpRequest& request) const {
  const auto result =
      clickhouse_client_
          ->Execute(cc_, "SELECT key, value FROM kv WHERE key = {}", key)
          .AsContainer<std::vector<KeyValueRow>>();
  if (result.size() != 1) {
    request.SetResponseStatus(server::http::HttpStatus::kNotFound);
    return {};
  }

  return result.front().value;
}

std::string KeyValue::PostValue(
    std::string_view key, const server::http::HttpRequest& request) const {
  const auto& value = request.GetArg("value");
  if (value.empty()) {
    throw server::handlers::ClientError{
        server::handlers::ExternalBody{"No 'value' query argument"}};
  }

  const std::vector<KeyValueRow> rows{{std::string{key}, value}};
  clickhouse_client_->InsertRows(cc_, "kv", {"key", "value"}, rows);

  request.SetResponseStatus(server::http::HttpStatus::kCreated);
  return value;
}

std::string KeyValue::DeleteValue(std::string_view key) const {
  clickhouse_client_->Execute(cc_, "ALTER TABLE kv DELETE WHERE key={}", key);

  return {};
}

}  // namespace chaos

int main(int argc, char* argv[]) {
  const auto component_list =
      components::MinimalServerComponentList()
          .Append<chaos::KeyValue>()
          .Append<components::Secdist>()
          .Append<components::DefaultSecdistProvider>()
          .Append<clients::dns::Component>()
          .Append<components::ClickHouse>("clickhouse-database");

  return utils::DaemonMain(argc, argv, component_list);
}

USERVER_NAMESPACE_BEGIN
namespace storages::clickhouse::io {

template <>
struct CppToClickhouse<chaos::KeyValueRow> final {
  using mapped_type = std::tuple<columns::StringColumn, columns::StringColumn>;
};

}  // namespace storages::clickhouse::io

USERVER_NAMESPACE_END
