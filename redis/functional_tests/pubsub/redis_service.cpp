#include <userver/testsuite/testsuite_support.hpp>
#include <userver/utest/using_namespace_userver.hpp>  // IWYU pragma: keep

#include <string>
#include <string_view>

#include <fmt/format.h>

#include <userver/clients/dns/component.hpp>
#include <userver/clients/http/component.hpp>
#include <userver/components/component.hpp>
#include <userver/components/minimal_server_component_list.hpp>
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
  static constexpr std::string_view kName = "handler-redis";

  ReadStoreReturn(const components::ComponentConfig& config,
                  const components::ComponentContext& context);

  ~ReadStoreReturn() final;

  std::string HandleRequestThrow(
      const server::http::HttpRequest& request,
      server::request::RequestContext&) const override;

  static yaml_config::Schema GetStaticConfigSchema();

 private:
  enum class Source { kQueued, kNonQueued };

  std::string Get(Source source) const;
  std::string Delete() const;

  std::shared_ptr<storages::redis::SubscribeClient> redis_client_;
  // Subscription with internal queue
  mutable std::vector<std::string> accumulated_data_with_queue_;
  storages::redis::SubscriptionToken token_with_queue_;
  // Subscription without internal queue
  storages::redis::CommandControl redis_cc_no_queue_;
  mutable std::vector<std::string> accumulated_data_no_queue_;
  storages::redis::SubscriptionToken token_no_queue_;
};

ReadStoreReturn::ReadStoreReturn(const components::ComponentConfig& config,
                                 const components::ComponentContext& context)
    : server::handlers::HttpHandlerBase(config, context),
      redis_client_{
          context.FindComponent<components::Redis>("key-value-database")
              .GetSubscribeClient(config["db"].As<std::string>())} {
  token_with_queue_ = redis_client_->Subscribe(
      "input_channel", [this](const auto&, const auto& data) {
        accumulated_data_with_queue_.push_back(data);
      });

  redis_cc_no_queue_.disable_subscription_queueing = true;
  token_no_queue_ = redis_client_->Subscribe(
      "input_channel",
      [this](const auto&, const auto& data) {
        accumulated_data_no_queue_.push_back(data);
      },
      redis_cc_no_queue_);
}

ReadStoreReturn::~ReadStoreReturn() {
  token_with_queue_.Unsubscribe();
  token_no_queue_.Unsubscribe();
}

std::string ReadStoreReturn::HandleRequestThrow(
    const server::http::HttpRequest& request,
    server::request::RequestContext& /*context*/) const {
  switch (request.GetMethod()) {
    case server::http::HttpMethod::kGet: {
      const auto& queued_arg = request.GetArg("queued");
      if (queued_arg.empty()) {
        throw server::handlers::ClientError(
            server::handlers::ExternalBody{"No 'queued' query argument"});
      }
      Source source{false};
      if (queued_arg == "yes") {
        source = Source::kQueued;
      } else if (queued_arg == "no") {
        source = Source::kNonQueued;
      } else {
        throw server::handlers::ClientError(
            server::handlers::ExternalBody{fmt::format(
                "'queued' argument must be yes or no, but is {} instead",
                queued_arg)});
      }
      return Get(source);
    }
    case server::http::HttpMethod::kDelete:
      return Delete();
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

std::string ReadStoreReturn::Get(Source source) const {
  formats::json::ValueBuilder builder{formats::common::Type::kObject};
  switch (source) {
    case Source::kQueued: {
      builder["data"] = accumulated_data_with_queue_;
      break;
    }
    case Source::kNonQueued: {
      builder["data"] = accumulated_data_no_queue_;
      break;
    }
  }
  return formats::json::ToString(builder.ExtractValue());
}

std::string ReadStoreReturn::Delete() const {
  accumulated_data_no_queue_.clear();
  accumulated_data_with_queue_.clear();
  return {};
}

}  // namespace chaos

int main(int argc, char* argv[]) {
  const auto component_list =
      components::MinimalServerComponentList()
          .Append<chaos::ReadStoreReturn>("handler-cluster")
          .Append<chaos::ReadStoreReturn>("handler-sentinel")
          .Append<components::HttpClient>()
          .Append<components::Secdist>()
          .Append<components::DefaultSecdistProvider>()
          .Append<components::Redis>("key-value-database")
          .Append<components::TestsuiteSupport>()
          .Append<server::handlers::TestsControl>()
          .Append<clients::dns::Component>();
  return utils::DaemonMain(argc, argv, component_list);
}
