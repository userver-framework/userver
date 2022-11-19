#include <userver/testsuite/testsuite_support.hpp>
#include <userver/utest/using_namespace_userver.hpp>

#include <fmt/format.h>
#include <string>
#include <string_view>

#include <userver/components/minimal_server_component_list.hpp>
#include <userver/ugrpc/client/client_factory_component.hpp>
#include <userver/formats/json.hpp>
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
};

}

namespace samples::etcd {

KeyValue::KeyValue(const components::ComponentConfig& config,
                   const components::ComponentContext& context)
    : server::handlers::HttpHandlerBase(config, context),
      etcd_client_{
          context.FindComponent<components::Etcd>("etcd")
            .GetClient()} {}

std::string KeyValue::HandleRequestThrow(
    const server::http::HttpRequest& request,
    server::request::RequestContext&) const {
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

std::string KeyValue::GetValue(std::string_view key,
                               const server::http::HttpRequest& request) const {
  auto result = etcd_client_->GetRange(std::string{key}, std::string{key}).Get();

  if (!result.size()) {
    request.SetResponseStatus(server::http::HttpStatus::kConflict);
    return {};
  }

  userver::formats::json::ValueBuilder ans{};
  ans[result[0].GetKey()] = result[0].GetValue();
  return formats::json::ToString(ans.ExtractValue());
}

std::string KeyValue::PostValue(
    std::string_view key, const server::http::HttpRequest& request) const {
  const auto& value = request.GetArg("value");
  etcd_client_->Put(std::string{key}, value);
  return {};
}

std::string KeyValue::DeleteValue(std::string_view key) const {
  etcd_client_->Delete(std::string{key});
  return {};
}

}

int main(int argc, char* argv[]) {
  const auto component_list =
      components::MinimalServerComponentList()
          .Append<samples::etcd::KeyValue>()
          .Append<components::Secdist>()
          .Append<userver::ugrpc::client::ClientFactoryComponent>()
          .Append<components::Etcd>("etcd")
          .Append<components::TestsuiteSupport>();
  return utils::DaemonMain(argc, argv, component_list);
}
