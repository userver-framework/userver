#include <userver/utest/using_namespace_userver.hpp>

#include <userver/clients/dns/component.hpp>
#include <userver/components/component.hpp>
#include <userver/components/minimal_server_component_list.hpp>
#include <userver/server/handlers/http_handler_base.hpp>
#include <userver/storages/mysql.hpp>
#include <userver/storages/secdist/component.hpp>
#include <userver/storages/secdist/provider_component.hpp>
#include <userver/utils/daemon_run.hpp>

namespace chaos {

struct KeyValueRow {
  std::string key;
  std::string value;
};

class KeyValue final : public server::handlers::HttpHandlerBase {
 public:
  static constexpr std::string_view kName{"handler-chaos"};

  KeyValue(const components::ComponentConfig& config,
           const components::ComponentContext& context)
      : server::handlers::HttpHandlerBase{config, context},
        mysql_{context.FindComponent<storages::mysql::Component>("key-value-db")
                   .GetCluster()} {}

  std::string HandleRequestThrow(
      const server::http::HttpRequest& request,
      server::request::RequestContext&) const override {
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

 private:
  static constexpr std::chrono::milliseconds kDefaultTimeout{2000};

  std::string GetValue(std::string_view key,
                       const server::http::HttpRequest& request) const {
    auto result = mysql_
                      ->Execute(cc_, storages::mysql::ClusterHostType::kPrimary,
                                "SELECT value FROM kv WHERE `key` = ?", key)
                      .AsOptionalSingleField<std::string>();

    if (!result.has_value()) {
      request.SetResponseStatus(server::http::HttpStatus::kNotFound);
      return {};
    }

    return std::move(*result);
  }

  std::string PostValue(std::string_view key,
                        const server::http::HttpRequest& request) const {
    const auto& value = request.GetArg("value");
    if (value.empty()) {
      throw server::handlers::ClientError{
          server::handlers::ExternalBody{"No 'value' query argument"}};
    }

    mysql_->Execute(cc_, storages::mysql::ClusterHostType::kPrimary,
                    "INSERT INTO kv(`key`, value) VALUES (?, ?)", key, value);

    request.SetResponseStatus(server::http::HttpStatus::kCreated);
    return value;
  }

  std::string DeleteValue(std::string_view key) const {
    mysql_->Execute(cc_, storages::mysql::ClusterHostType::kPrimary,
                    "DELETE FROM kv WHERE `key` = ?", key);

    return {};
  }

  const std::shared_ptr<storages::mysql::Cluster> mysql_;
  storages::mysql::CommandControl cc_{kDefaultTimeout};
};

}  // namespace chaos

int main(int argc, char* argv[]) {
  const auto component_list =
      components::MinimalServerComponentList()
          .Append<chaos::KeyValue>()
          .Append<components::Secdist>()
          .Append<components::DefaultSecdistProvider>()
          .Append<clients::dns::Component>()
          .Append<storages::mysql::Component>("key-value-db");

  return utils::DaemonMain(argc, argv, component_list);
}
