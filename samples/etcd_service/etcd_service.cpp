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
  std::string GetValue(const std::string& key,
                       const server::http::HttpRequest& request) const;
  std::string PostValue(const std::string& key,
                        const server::http::HttpRequest& request) const;
  std::string DeleteValue(const std::string& key,
                          const server::http::HttpRequest& request) const;

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
    request.SetResponseStatus(server::http::HttpStatus::kBadRequest);
    return {};
  }

  switch (request.GetMethod()) {
    case server::http::HttpMethod::kGet:
      return GetValue(key, request);
    case server::http::HttpMethod::kPost:
      return PostValue(key, request);
    case server::http::HttpMethod::kDelete:
      return DeleteValue(key, request);
    default:
      throw server::handlers::ClientError(server::handlers::ExternalBody{
          fmt::format("Unsupported method {}", request.GetMethod())});
  }
}

std::string KeyValue::GetValue(const std::string& key,
                               const server::http::HttpRequest& request) const {
  auto key_end = std::make_optional<std::string>(request.GetArg("key_end"));
  if (key_end->empty()) {
    key_end = std::nullopt;
  }

  auto result = etcd_client_->Get(key, key_end);
  if (!result.size()) {
    request.SetResponseStatus(server::http::HttpStatus::kNotFound);
    return {};
  }

  userver::formats::json::ValueBuilder ans{};
  for (auto item : result) {
    ans[item.GetKey()] = item.GetValue();
  }

  return formats::json::ToString(ans.ExtractValue());
}

std::string KeyValue::PostValue(const std::string& key,
                                const server::http::HttpRequest& request) const {
  const auto& value = request.GetArg("value");
  if (value.empty()) {
    request.SetResponseStatus(server::http::HttpStatus::kBadRequest);
    return {};
  }

  etcd_client_->Put(key, value);
  return {};
}

std::string KeyValue::DeleteValue(const std::string& key,
                                  const server::http::HttpRequest& request) const {
  auto key_end = std::make_optional<std::string>(request.GetArg("key_end"));
  if (key_end->empty()) {
    key_end = std::nullopt;
  }

  etcd_client_->Delete(key, key_end);
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
