#include <userver/testsuite/testsuite_support.hpp>
#include <userver/utest/using_namespace_userver.hpp>  // IWYU pragma: keep

#include <atomic>
#include <string>
#include <string_view>

#include <fmt/format.h>

#include <userver/clients/dns/component.hpp>
#include <userver/clients/http/component.hpp>
#include <userver/components/minimal_server_component_list.hpp>
#include <userver/server/handlers/http_handler_base.hpp>
#include <userver/server/handlers/server_monitor.hpp>
#include <userver/server/handlers/tests_control.hpp>
#include <userver/storages/redis/client.hpp>
#include <userver/storages/redis/component.hpp>
#include <userver/storages/redis/subscribe_client.hpp>
#include <userver/storages/secdist/component.hpp>
#include <userver/storages/secdist/provider_component.hpp>
#include <userver/utils/daemon_run.hpp>

namespace chaos {

constexpr char kPostChannel[] = "post_channel0";

class KeyValue final : public server::handlers::HttpHandlerBase {
 public:
  static constexpr std::string_view kName = "handler-metrics";

  KeyValue(const components::ComponentConfig& config,
           const components::ComponentContext& context);

  ~KeyValue() override;

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
  storages::redis::SubscribeClientPtr subscribe_client_;
  storages::redis::SubscriptionToken subscription_token_;
  storages::redis::CommandControl redis_cc_{};
  std::atomic_int values_posted{0};
};

KeyValue::KeyValue(const components::ComponentConfig& config,
                   const components::ComponentContext& context)
    : server::handlers::HttpHandlerBase(config, context),
      redis_client_{
          context.FindComponent<components::Redis>("key-value-database")
              .GetClient("metrics_test")},
      subscribe_client_{
          context.FindComponent<components::Redis>("key-value-database")
              .GetSubscribeClient("metrics_test")} {
  redis_cc_.force_request_to_master = true;
  subscription_token_ = subscribe_client_->Subscribe(
      kPostChannel,
      [&](const std::string& channel, const std::string& message) {
        LOG_INFO() << channel << ": " << message;
        ++values_posted;
      });
}

KeyValue::~KeyValue() { subscription_token_.Unsubscribe(); }

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

std::string KeyValue::GetValue(std::string_view key,
                               const server::http::HttpRequest& request) const {
  const auto result = redis_client_->Get(std::string{key}, redis_cc_).Get();
  if (!result) {
    request.SetResponseStatus(server::http::HttpStatus::kNotFound);
    return {};
  }
  return *result;
}

std::string KeyValue::PostValue(
    std::string_view key, const server::http::HttpRequest& request) const {
  const auto& value = request.GetArg("value");
  const auto result =
      redis_client_->SetIfNotExist(std::string{key}, value, redis_cc_).Get();
  if (!result) {
    request.SetResponseStatus(server::http::HttpStatus::kConflict);
    return {};
  }

  redis_client_->Publish(kPostChannel, value, redis_cc_);

  request.SetResponseStatus(server::http::HttpStatus::kCreated);
  return std::string{value};
}

std::string KeyValue::DeleteValue(std::string_view key) const {
  const auto result = redis_client_->Del(std::string{key}, redis_cc_).Get();
  return std::to_string(result);
}

}  // namespace chaos

int main(int argc, char* argv[]) {
  const auto component_list =
      components::MinimalServerComponentList()
          .Append<server::handlers::ServerMonitor>()
          .Append<chaos::KeyValue>()
          .Append<components::Secdist>()
          .Append<components::DefaultSecdistProvider>()
          .Append<components::Redis>("key-value-database")
          .Append<components::TestsuiteSupport>()
          .Append<components::HttpClient>()
          .Append<clients::dns::Component>()
          .Append<server::handlers::TestsControl>();
  return utils::DaemonMain(argc, argv, component_list);
}
