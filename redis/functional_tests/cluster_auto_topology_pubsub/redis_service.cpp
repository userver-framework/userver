#include <chrono>
#include <userver/testsuite/testsuite_support.hpp>
#include <userver/utest/using_namespace_userver.hpp>  // IWYU pragma: keep

#include <string>
#include <string_view>

#include <fmt/format.h>

#include <userver/clients/dns/component.hpp>
#include <userver/clients/http/component.hpp>
#include <userver/components/component.hpp>
#include <userver/components/minimal_server_component_list.hpp>
#include <userver/dynamic_config/client/component.hpp>
#include <userver/dynamic_config/updater/component.hpp>
#include <userver/formats/json/serialize.hpp>
#include <userver/formats/json/value.hpp>
#include <userver/formats/serialize/common_containers.hpp>
#include <userver/server/handlers/http_handler_base.hpp>
#include <userver/server/handlers/tests_control.hpp>
#include <userver/storages/redis/client.hpp>
#include <userver/storages/redis/command_control.hpp>
#include <userver/storages/redis/command_options.hpp>
#include <userver/storages/redis/component.hpp>
#include <userver/storages/redis/impl/keyshard.hpp>
#include <userver/storages/redis/subscribe_client.hpp>
#include <userver/storages/secdist/component.hpp>
#include <userver/storages/secdist/provider_component.hpp>
#include <userver/utils/daemon_run.hpp>
#include <userver/utils/periodic_task.hpp>
#include <userver/yaml_config/merge_schemas.hpp>

namespace chaos {

const constexpr size_t kInputChannelsCount = 5;
const std::string kInputChannel = "input_channel";
const std::string kInputChannelName0 = kInputChannel + "@" + std::to_string(0);

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
  /// input_channel -> vector<messages>
  using Data = concurrent::Variable<
      std::unordered_map<std::string, std::vector<std::string>>>;

  std::string Get(const server::http::HttpRequest& request) const;
  std::string Get() const;
  std::string Get(const std::string& channel_name) const;
  std::string Delete() const;

  std::shared_ptr<storages::redis::Client> redis_client_;
  std::shared_ptr<storages::redis::SubscribeClient> redis_subscribe_client_;
  // Subscription with internal queue
  mutable Data accumulated_data_with_queue_;
  std::array<storages::redis::SubscriptionToken, kInputChannelsCount> tokens_;

  utils::PeriodicTask publisher_task_;
};

ReadStoreReturn::ReadStoreReturn(const components::ComponentConfig& config,
                                 const components::ComponentContext& context)
    : server::handlers::HttpHandlerBase(config, context),
      redis_client_{
          context.FindComponent<components::Redis>("key-value-database")
              .GetClient(config["db"].As<std::string>())},
      redis_subscribe_client_{
          context.FindComponent<components::Redis>("key-value-database")
              .GetSubscribeClient("redis-cluster-subscribe")} {
  auto callback = [this](const auto& channel_name, const auto& data) {
    UASSERT(engine::current_task::IsTaskProcessorThread());
    auto locked = accumulated_data_with_queue_.Lock();
    (*locked)[channel_name].push_back(data);
  };
  for (size_t i = 0; i < kInputChannelsCount; ++i) {
    const auto channel_name = kInputChannel + "@" + std::to_string(i);
    tokens_[i] = redis_subscribe_client_->Subscribe(channel_name, callback);
  }

  const utils::PeriodicTask::Settings settings(std::chrono::milliseconds(1000));
  publisher_task_.Start("publisher", settings, [this] {
    redis_client_->Publish("periodic_publish", "42", redis::CommandControl(),
                           storages::redis::PubShard::kRoundRobin);
  });
}

ReadStoreReturn::~ReadStoreReturn() {
  publisher_task_.Stop();
  for (auto& token : tokens_) token.Unsubscribe();
}

std::string ReadStoreReturn::HandleRequestThrow(
    const server::http::HttpRequest& request,
    server::request::RequestContext& /*context*/) const {
  switch (request.GetMethod()) {
    case server::http::HttpMethod::kGet:
      return Get(request);
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

std::string ReadStoreReturn::Get(
    const server::http::HttpRequest& request) const {
  const auto& shard_count = request.GetArg("shard_count");
  if (!shard_count.empty()) {
    return std::to_string(redis_subscribe_client_->ShardsCount());
  }

  {
    const auto& publish_msg = request.GetArg("publish");
    if (!publish_msg.empty()) {
      const auto& shard_str = request.GetArg("shard");
      if (!shard_str.empty()) {
        /// Publish message to specified shard
        const auto shard = std::stol(shard_str);
        auto shard_client = redis_client_->GetClientForShard(shard);
        shard_client->Publish("output_channel", publish_msg,
                              redis::CommandControl());
        return {};
      }
      /// Publish to any accessible shard
      redis_client_->Publish("output_channel", publish_msg,
                             redis::CommandControl());
      return {};
    }
  }

  {
    const auto& publish_msg = request.GetArg("spublish");
    if (!publish_msg.empty()) {
      const auto& channel = request.GetArg("channel");
      if (!channel.empty()) {
        redis_client_->Spublish(channel, publish_msg, redis::CommandControl());
        return {};
      }
      return {};
    }
  }

  const auto& input_channel = request.GetArg("read");
  if (!input_channel.empty()) {
    return Get(input_channel);
  }

  return Get();
}

std::string ReadStoreReturn::Get() const { return Get(kInputChannelName0); }

std::string ReadStoreReturn::Get(const std::string& channel_name) const {
  formats::json::ValueBuilder builder{formats::common::Type::kObject};
  auto locked = accumulated_data_with_queue_.Lock();
  builder["data"] = (*locked)[channel_name];
  return formats::json::ToString(builder.ExtractValue());
}

std::string ReadStoreReturn::Delete() const {
  auto locked = accumulated_data_with_queue_.Lock();
  locked->clear();
  return {};
}

}  // namespace chaos

int main(int argc, char* argv[]) {
  const auto component_list =
      components::MinimalServerComponentList()
          .Append<chaos::ReadStoreReturn>("handler-cluster")
          .Append<components::HttpClient>()
          .Append<components::Secdist>()
          .Append<components::DefaultSecdistProvider>()
          .Append<components::Redis>("key-value-database")
          .Append<components::TestsuiteSupport>()
          .Append<server::handlers::TestsControl>()
          .Append<components::DynamicConfigClient>()
          .Append<components::DynamicConfigClientUpdater>()
          .Append<clients::dns::Component>();
  return utils::DaemonMain(argc, argv, component_list);
}
