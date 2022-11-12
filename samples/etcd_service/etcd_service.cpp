#include <userver/testsuite/testsuite_support.hpp>
#include <userver/utest/using_namespace_userver.hpp>  // IWYU pragma: keep

#include <fmt/format.h>
#include <string>
#include <string_view>

/// [Etcd service sample - component]
#include <userver/components/minimal_server_component_list.hpp>
#include <userver/server/handlers/http_handler_base.hpp>
#include <userver/storages/etcd/client.hpp>
#include <userver/storages/etcd/component.hpp>
#include <userver/storages/secdist/component.hpp>
#include <userver/utils/daemon_run.hpp>

namespace samples::etcd {
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

  storages::etcd::ClientPtr etcd_client_;
  storages::etcd::CommandControl etcd_cc_;
};

}  // namespace samples::etcd
/// [Etcd service sample - component]

namespace samples::etcd {

/// [Etcd service sample - component constructor]
KeyValue::KeyValue(const components::ComponentConfig& config,
                   const components::ComponentContext& context)
    : server::handlers::HttpHandlerBase(config, context),
      etcd_client_{
          context.FindComponent<components::Etcd>("key-value-database")
              .GetClient("taxi-tmp")} {}
/// [Etcd service sample - component constructor]

/// [Etcd service sample - HandleRequestThrow]
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
/// [Etcd service sample - HandleRequestThrow]

/// [Etcd service sample - GetValue]
std::string KeyValue::GetValue(std::string_view key,
                               const server::http::HttpRequest& request) const {
  const auto result = etcd_client_->Get(std::string{key}, etcd_cc_).Get();
  if (!result) {
    request.SetResponseStatus(server::http::HttpStatus::kNotFound);
    return {};
  }
  return *result;
}
/// [Etcd service sample - GetValue]

/// [Etcd service sample - PostValue]
std::string KeyValue::PostValue(
    std::string_view key, const server::http::HttpRequest& request) const {
  const auto& value = request.GetArg("value");
  const auto result =
      etcd_client_->SetIfNotExist(std::string{key}, value, etcd_cc_).Get();
  if (!result) {
    request.SetResponseStatus(server::http::HttpStatus::kConflict);
    return {};
  }

  request.SetResponseStatus(server::http::HttpStatus::kCreated);
  return std::string{value};
}
/// [Etcd service sample - PostValue]

/// [Etcd service sample - DeleteValue]
std::string KeyValue::DeleteValue(std::string_view key) const {
  const auto result = etcd_client_->Del(std::string{key}, etcd_cc_).Get();
  return std::to_string(result);
}
/// [Etcd service sample - DeleteValue]
}  // namespace samples::etcd

/// [Etcd sample - main]
int main(int argc, char* argv[]) {
  const auto component_list =
      components::MinimalServerComponentList()
          .Append<samples::etcd::KeyValue>()
          .Append<components::Secdist>()
          .Append<components::Etcd>("key-value-database")
          .Append<components::TestsuiteSupport>();
  return utils::DaemonMain(argc, argv, component_list);
}
/// [Etcd service sample - main]
