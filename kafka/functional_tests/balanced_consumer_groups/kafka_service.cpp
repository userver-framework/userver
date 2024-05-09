#include <atomic>
#include <unordered_map>

#include <userver/utest/using_namespace_userver.hpp>

#include <userver/components/component_config.hpp>
#include <userver/components/component_context.hpp>
#include <userver/components/component_list.hpp>
#include <userver/components/minimal_server_component_list.hpp>

#include <userver/clients/dns/component.hpp>
#include <userver/clients/http/component.hpp>
#include <userver/concurrent/variable.hpp>
#include <userver/engine/wait_all_checked.hpp>
#include <userver/formats/json/serialize_container.hpp>
#include <userver/formats/json/value.hpp>
#include <userver/formats/json/value_builder.hpp>

#include <userver/server/handlers/http_handler_json_base.hpp>
#include <userver/server/handlers/tests_control.hpp>

#include <userver/storages/secdist/component.hpp>
#include <userver/storages/secdist/provider_component.hpp>

#include <userver/kafka/components/consumer_component.hpp>

#include <userver/testsuite/testsuite_support.hpp>

#include <userver/utils/daemon_run.hpp>
#include <userver/utils/lazy_prvalue.hpp>
#include <userver/yaml_config/merge_schemas.hpp>

#include <fmt/ranges.h>

namespace functional_tests {

constexpr std::string_view kConsumersListFieldName = "consumers_list";

constexpr std::string_view kReqConsumerArgName = "consumer_name";

constexpr std::string_view kErrorConsumerNotFound = R"(
  {
    "error": "Consumer not found"
  }
)";

constexpr std::string_view kMessageToFail = "please-start-failing";
constexpr std::string_view kFaultyConsumer = "kafka-consumer-second";

class HandlerKafkaConsumerGroups final
    : public server::handlers::HttpHandlerJsonBase {
 public:
  static constexpr std::string_view kName = "handler-kafka-consumer-groups";

  HandlerKafkaConsumerGroups(const components::ComponentConfig& config,
                             const components::ComponentContext& context);

  formats::json::Value HandleRequestJsonThrow(
      const server::http::HttpRequest& request,
      const formats::json::Value& request_json,
      server::request::RequestContext& context) const override;

  static yaml_config::Schema GetStaticConfigSchema();

 private:
  void StartConsuming(const std::string& consumer_name) const;
  void StopConsuming(const std::string& consumer_name) const;

  void Consume(const std::string& consumer_name,
               kafka::MessageBatchView messages) const;
  formats::json::Value ReleaseMessages(const std::string& consumer_name) const;

  bool HasConsumer(const std::string& consumer_name) const;

  kafka::ConsumerScope& GetConsumerScope(
      const std::string& consumer_name) const;

  void DumpCurrentConsumed(const std::string& consumer_name);

 private:
  using ConsumerByName = std::unordered_map<std::string, kafka::ConsumerScope>;
  using MessagesByConsumer =
      std::unordered_map<std::string, std::vector<formats::json::Value>>;

  mutable concurrent::Variable<ConsumerByName> consumer_by_name_;
  mutable concurrent::Variable<MessagesByConsumer> messages_by_consumer_;

  mutable std::atomic<bool> should_fail_{true};
};

}  // namespace functional_tests

namespace functional_tests {

namespace {

formats::json::Value Serialize(const kafka::Message& message,
                               formats::serialize::To<formats::json::Value>) {
  formats::json::ValueBuilder builder{formats::common::Type::kObject};
  builder["topic"] = message.GetTopic();
  builder["key"] = message.GetKey();
  builder["payload"] = message.GetPayload();
  builder["partition"] = message.GetPartition();

  return builder.ExtractValue();
}

}  // namespace

HandlerKafkaConsumerGroups::HandlerKafkaConsumerGroups(
    const components::ComponentConfig& config,
    const components::ComponentContext& context)
    : server::handlers::HttpHandlerJsonBase(config, context) {
  auto consumer_by_name = consumer_by_name_.Lock();
  auto messages_by_consumer = messages_by_consumer_.Lock();
  for (const auto& consumer_name :
       config[kConsumersListFieldName].As<std::vector<std::string>>()) {
    consumer_by_name->emplace(
        consumer_name, utils::LazyPrvalue([&context, &consumer_name] {
          return context.FindComponent<kafka::ConsumerComponent>(consumer_name)
              .GetConsumer();
        }));
    messages_by_consumer->emplace(consumer_name,
                                  std::vector<formats::json::Value>{});
  }
}

formats::json::Value HandlerKafkaConsumerGroups::HandleRequestJsonThrow(
    const server::http::HttpRequest& request,
    [[maybe_unused]] const formats::json::Value& request_json,
    [[maybe_unused]] server::request::RequestContext& context) const {
  const std::string& consumer_name = request.GetPathArg(kReqConsumerArgName);

  if (!HasConsumer(consumer_name)) {
    request.SetResponseStatus(server::http::HttpStatus::kBadRequest);

    return formats::json::FromString(kErrorConsumerNotFound);
  }

  switch (request.GetMethod()) {
    case server::http::HttpMethod::kPut:
      StartConsuming(consumer_name);
      return {};
    case server::http::HttpMethod::kDelete:
      StopConsuming(consumer_name);
      return {};
    case server::http::HttpMethod::kPatch:
      should_fail_.store(false);
      return {};
    case server::http::HttpMethod::kPost:
      return ReleaseMessages(consumer_name);
    default:
      throw std::runtime_error{
          "Unexpected request method not stated in static config"};
  }
}

void HandlerKafkaConsumerGroups::StartConsuming(
    const std::string& consumer_name) const {
  auto& consumer = GetConsumerScope(consumer_name);

  consumer.Start([this, consumer_name = consumer_name,
                  &consumer](kafka::MessageBatchView messages) {
    Consume(consumer_name, messages);
    consumer.AsyncCommit();
  });
}

void HandlerKafkaConsumerGroups::StopConsuming(
    const std::string& consumer_name) const {
  GetConsumerScope(consumer_name).Stop();
}

void HandlerKafkaConsumerGroups::Consume(
    const std::string& consumer_name, kafka::MessageBatchView messages) const {
  auto messages_by_consumer = messages_by_consumer_.Lock();

  auto messages_it = messages_by_consumer->find(consumer_name);
  UINVARIANT(
      messages_it != messages_by_consumer->end(),
      fmt::format(
          "Expected to find consumer '{}' (according to HasConsumer check)",
          consumer_name));

  auto& messages_storage = messages_it->second;
  for (const auto& message : messages) {
    if (should_fail_.load() && message.GetPayload() == kMessageToFail &&
        consumer_name == kFaultyConsumer) {
      LOG_WARNING() << "Received fail message!";
      throw std::runtime_error{"Bad messages, i am going to fail forever!"};
    }

    messages_storage.emplace_back(
        Serialize(message, formats::serialize::To<formats::json::Value>{}));
  }

  LOG_DEBUG() << fmt::format("Consumer '{}' consumed: {}", consumer_name,
                             fmt::join(messages_storage, ", "));
}

formats::json::Value HandlerKafkaConsumerGroups::ReleaseMessages(
    const std::string& consumer_name) const {
  formats::json::ValueBuilder builder{formats::common::Type::kObject};

  std::vector<formats::json::Value> consumed_messages;
  {
    auto messages_by_consumer = messages_by_consumer_.Lock();

    auto messages_it = messages_by_consumer->find(consumer_name);
    if (messages_it != messages_by_consumer->end()) {
      auto& messages_storage = messages_it->second;

      consumed_messages.swap(messages_storage);
    }
  }

  builder["messages"] = consumed_messages;

  return builder.ExtractValue();
}

bool HandlerKafkaConsumerGroups::HasConsumer(
    const std::string& consumer_name) const {
  auto consumer_by_name = consumer_by_name_.Lock();

  return consumer_by_name->find(consumer_name) != consumer_by_name->end();
}

kafka::ConsumerScope& HandlerKafkaConsumerGroups::GetConsumerScope(
    const std::string& consumer_name) const {
  auto consumer_by_name = consumer_by_name_.Lock();

  return consumer_by_name->at(consumer_name);
}

yaml_config::Schema HandlerKafkaConsumerGroups::GetStaticConfigSchema() {
  return yaml_config::MergeSchemas<server::handlers::HttpHandlerJsonBase>(R"(
type: object
description: Handler of Kafka consumer groups
additionalProperties: false
properties:
    consumers_list:
        type: array
        description: list of consumer names
        items:
          type: string
          description: consumer name
)");
}

}  // namespace functional_tests

int main(int argc, char* argv[]) {
  const auto components_list =
      components::MinimalServerComponentList()
          .Append<kafka::ConsumerComponent>("kafka-consumer-first")
          .Append<kafka::ConsumerComponent>("kafka-consumer-second")
          .Append<components::TestsuiteSupport>()
          .Append<components::Secdist>()
          .Append<components::DefaultSecdistProvider>()
          .Append<components::HttpClient>()
          .Append<clients::dns::Component>()
          .Append<server::handlers::TestsControl>()
          .Append<functional_tests::HandlerKafkaConsumerGroups>();

  return utils::DaemonMain(argc, argv, components_list);
}
