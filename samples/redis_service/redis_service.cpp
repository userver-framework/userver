#include <userver/testsuite/testsuite_support.hpp>
#include <userver/utest/using_namespace_userver.hpp>  // IWYU pragma: keep

#include <fmt/format.h>
#include <string>
#include <string_view>

/// [Redis service sample - component]
#include <userver/components/minimal_server_component_list.hpp>
#include <userver/server/handlers/http_handler_base.hpp>
#include <userver/storages/redis/client.hpp>
#include <userver/storages/redis/component.hpp>
#include <userver/storages/secdist/component.hpp>
#include <userver/utils/daemon_run.hpp>

namespace samples::redis {

class KeyValue final : public server::handlers::HttpHandlerBase {
 public:
  static constexpr std::string_view kName = "handler-key-value";

  KeyValue(const components::ComponentConfig& config,
           const components::ComponentContext& context);

  std::string HandleRequestThrow(
      const server::http::HttpRequest& request,
      server::request::RequestContext&) const override;

 private:
  std::string GetValue(std::string_view key,
                       const server::http::HttpRequest& request) const;
  std::string PostValue(std::string_view key,
                        const server::http::HttpRequest& request) const;
  std::string DeleteValue(std::string_view key) const;

  storages::redis::ClientPtr redis_client_;
  storages::redis::CommandControl redis_cc_;
};

}  // namespace samples::redis
/// [Redis service sample - component]

namespace samples::redis {

/// [Redis service sample - component constructor]
KeyValue::KeyValue(const components::ComponentConfig& config,
                   const components::ComponentContext& context)
    : server::handlers::HttpHandlerBase(config, context),
      redis_client_{
          context.FindComponent<components::Redis>("key-value-database")
              .GetClient("taxi-tmp")} {}
/// [Redis service sample - component constructor]

/// [Redis service sample - HandleRequestThrow]
std::string KeyValue::HandleRequestThrow(
    const server::http::HttpRequest& request,
    server::request::RequestContext& /*context*/) const {
  const auto& key = request.GetArg("key");
  if (key.empty()) {
    throw server::handlers::ClientError(
        server::handlers::ExternalBody{"No 'key' query argument"});
  }

  switch (request.GetMethod()) {
    case server::http::HttpMethod::kGet:
      return GetValue(key, request);
    case server::http::HttpMethod::kPost:
      return PostValue(key, request);
    case server::http::HttpMethod::kDelete:
      return DeleteValue(key);
    default:
      throw server::handlers::ClientError(server::handlers::ExternalBody{
          fmt::format("Unsupported method {}", request.GetMethod())});
  }
}
/// [Redis service sample - HandleRequestThrow]

/// [Redis service sample - GetValue]
std::string KeyValue::GetValue(std::string_view key,
                               const server::http::HttpRequest& request) const {
  const auto result = redis_client_->Get(std::string{key}, redis_cc_).Get();
  if (!result) {
    request.SetResponseStatus(server::http::HttpStatus::kNotFound);
    return {};
  }
  return *result;
}
/// [Redis service sample - GetValue]

/// [Redis service sample - PostValue]
std::string KeyValue::PostValue(
    std::string_view key, const server::http::HttpRequest& request) const {
  const auto& value = request.GetArg("value");
  const auto result =
      redis_client_->SetIfNotExist(std::string{key}, value, redis_cc_).Get();
  if (!result) {
    request.SetResponseStatus(server::http::HttpStatus::kConflict);
    return {};
  }

  request.SetResponseStatus(server::http::HttpStatus::kCreated);
  return std::string{value};
}
/// [Redis service sample - PostValue]

/// [Redis service sample - DeleteValue]
std::string KeyValue::DeleteValue(std::string_view key) const {
  const auto result = redis_client_->Del(std::string{key}, redis_cc_).Get();
  return std::to_string(result);
}
/// [Redis service sample - DeleteValue]

}  // namespace samples::redis

/// [Redis service sample - main]
int main(int argc, char* argv[]) {
  const auto component_list =
      components::MinimalServerComponentList()
          .Append<samples::redis::KeyValue>()
          .Append<components::Secdist>()
          .Append<components::Redis>("key-value-database")
          .Append<components::TestsuiteSupport>();
  return utils::DaemonMain(argc, argv, component_list);
}
/// [Redis service sample - main]
