#include <userver/testsuite/testsuite_support.hpp>
#include <userver/utest/using_namespace_userver.hpp>  // IWYU pragma: keep

#include <string>
#include <string_view>

#include <fmt/format.h>

#include <userver/clients/dns/component.hpp>
#include <userver/clients/http/component.hpp>
#include <userver/components/component.hpp>
#include <userver/components/minimal_server_component_list.hpp>
#include <userver/concurrent/variable.hpp>
#include <userver/formats/json/serialize.hpp>
#include <userver/formats/json/value.hpp>
#include <userver/formats/serialize/common_containers.hpp>
#include <userver/server/handlers/http_handler_base.hpp>
#include <userver/server/handlers/tests_control.hpp>
#include <userver/storages/redis/command_options.hpp>
#include <userver/storages/redis/component.hpp>
#include <userver/storages/redis/subscribe_client.hpp>
#include <userver/storages/secdist/component.hpp>
#include <userver/storages/secdist/provider_component.hpp>
#include <userver/utils/daemon_run.hpp>
#include <userver/yaml_config/merge_schemas.hpp>

namespace chaos {

class ReadStoreReturn final : public server::handlers::HttpHandlerBase {
 public:
  ReadStoreReturn(const components::ComponentConfig& config,
                  const components::ComponentContext& context);

  ~ReadStoreReturn() final;

  std::string HandleRequestThrow(
      const server::http::HttpRequest& request,
      server::request::RequestContext&) const override;

  static yaml_config::Schema GetStaticConfigSchema();

 private:
  std::string Get() const;
  std::string Delete() const;
  std::string Update() const;

  storages::redis::SubscriptionToken Subscribe() const;

  using Data = concurrent::Variable<std::vector<std::string>>;

  const std::shared_ptr<storages::redis::SubscribeClient> redis_client_;

  mutable Data accumulated_data_with_queue_;
  mutable storages::redis::SubscriptionToken token_with_queue_;
};

ReadStoreReturn::ReadStoreReturn(const components::ComponentConfig& config,
                                 const components::ComponentContext& context)
    : server::handlers::HttpHandlerBase(config, context),
      redis_client_{
          context.FindComponent<components::Redis>("key-value-database")
              .GetSubscribeClient(config["db"].As<std::string>())},
      token_with_queue_(Subscribe()) {}

ReadStoreReturn::~ReadStoreReturn() { token_with_queue_.Unsubscribe(); }

std::string ReadStoreReturn::HandleRequestThrow(
    const server::http::HttpRequest& request,
    server::request::RequestContext& /*context*/) const {
  switch (request.GetMethod()) {
    case server::http::HttpMethod::kGet:
      return Get();
    case server::http::HttpMethod::kDelete:
      return Delete();
    case server::http::HttpMethod::kPut:
      return Update();
    default:
      throw server::handlers::ClientError(server::handlers::ExternalBody{
          fmt::format("Unsupported method {}", request.GetMethod())});
  }
}

yaml_config::Schema ReadStoreReturn::GetStaticConfigSchema() {
  return yaml_config::MergeSchemas<HandlerBase>(R"(
type: object
description: ReadStoreReturn handler schema
additionalProperties: false
properties:
    db:
        type: string
        description: redis database name
)");
}

std::string ReadStoreReturn::Get() const {
  formats::json::ValueBuilder builder{formats::common::Type::kObject};
  const auto locked = accumulated_data_with_queue_.Lock();
  builder["data"] = *locked;

  return formats::json::ToString(builder.ExtractValue());
}

std::string ReadStoreReturn::Delete() const {
  auto locked = accumulated_data_with_queue_.Lock();
  locked->clear();
  return {};
}

std::string ReadStoreReturn::Update() const {
  auto temp = Subscribe();
  token_with_queue_.Unsubscribe();
  token_with_queue_ = std::move(temp);
  return {};
}

storages::redis::SubscriptionToken ReadStoreReturn::Subscribe() const {
  return redis_client_->Subscribe(
      "input_channel", [this](const auto&, const auto& data) {
        UASSERT(engine::current_task::IsTaskProcessorThread());
        auto locked = accumulated_data_with_queue_.Lock();
        locked->push_back(data);
      });
}

}  // namespace chaos

int main(int argc, char* argv[]) {
  const auto component_list =
      components::MinimalServerComponentList()
          .Append<chaos::ReadStoreReturn>("handler-cluster")
          .Append<chaos::ReadStoreReturn>("handler-sentinel")
          .Append<chaos::ReadStoreReturn>("handler-sentinel-with-master")
          .Append<components::HttpClient>()
          .Append<components::Secdist>()
          .Append<components::DefaultSecdistProvider>()
          .Append<components::Redis>("key-value-database")
          .Append<components::TestsuiteSupport>()
          .Append<server::handlers::TestsControl>()
          .Append<clients::dns::Component>();
  return utils::DaemonMain(argc, argv, component_list);
}
