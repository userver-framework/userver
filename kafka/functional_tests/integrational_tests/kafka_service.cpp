#include <unordered_map>

#include <userver/utest/using_namespace_userver.hpp>

#include <userver/components/component_config.hpp>
#include <userver/components/component_context.hpp>
#include <userver/components/component_list.hpp>
#include <userver/components/minimal_server_component_list.hpp>

#include <userver/clients/dns/component.hpp>
#include <userver/clients/http/component.hpp>
#include <userver/concurrent/variable.hpp>
#include <userver/formats/json/serialize_container.hpp>
#include <userver/formats/json/value.hpp>
#include <userver/formats/json/value_builder.hpp>

#include <userver/server/handlers/http_handler_json_base.hpp>
#include <userver/server/handlers/tests_control.hpp>

#include <userver/storages/secdist/component.hpp>
#include <userver/storages/secdist/provider_component.hpp>

#include <userver/kafka/components/consumer_component.hpp>
#include <userver/kafka/components/producer_component.hpp>
#include <userver/kafka/components/tests_messages_component.hpp>

#include <userver/testsuite/testsuite_support.hpp>

#include <userver/utils/daemon_run.hpp>
#include <userver/yaml_config/merge_schemas.hpp>

namespace functional_tests {

constexpr auto kProducersListFieldName = "producers_list";

constexpr std::string_view kReqProducerFieldName = "producer";
constexpr std::string_view kReqTopicFieldName = "producer";
constexpr std::string_view kReqKeyFieldName = "key";
constexpr std::string_view kReqPayloadFieldName = "payload";

constexpr std::string_view kMessageSend = R"(
  {
    "message": "Message send successfully"
  }
)";
constexpr std::string_view kErrorUnknown = R"(
  {
    "error": "Internal error"
  }
)";
constexpr std::string_view kErrorMembersNotSet = R"(
  {
    "error": "Expected body has 'producer', `key` and `message` fields"
  }
)";
constexpr std::string_view kErrorProducerNotFound = R"(
  {
    "error": "Given producer not found"
  }
)";

class HandlerKafkaConsumer final
    : public server::handlers::HttpHandlerJsonBase {
 public:
  static constexpr std::string_view kName = "handler-kafka-consumer";

  HandlerKafkaConsumer(const components::ComponentConfig& config,
                       const components::ComponentContext& context);

  formats::json::Value HandleRequestJsonThrow(
      const server::http::HttpRequest& request,
      const formats::json::Value& request_json,
      server::request::RequestContext& context) const override;

 private:
  std::vector<formats::json::Value> ReleaseMessages() const;

  void Consume(std::vector<kafka::MessagePolled>&& messages);

 private:
  mutable concurrent::Variable<std::vector<formats::json::Value>> messages_;

  // Subscriptions must be the last fields! Add new fields above this comment.
  kafka::ConsumerScope consumer_;
};

class HandlerKafkaProducers final
    : public server::handlers::HttpHandlerJsonBase {
 public:
  static constexpr std::string_view kName = "handler-kafka-producers";

  HandlerKafkaProducers(const components::ComponentConfig& config,
                        const components::ComponentContext& context);

  formats::json::Value HandleRequestJsonThrow(
      const server::http::HttpRequest& request,
      const formats::json::Value& request_json,
      server::request::RequestContext& context) const override;

  static yaml_config::Schema GetStaticConfigSchema();

 private:
  std::unordered_map<std::string, kafka::Producer&> producer_by_topic_;
};

}  // namespace functional_tests

namespace functional_tests {

namespace {

formats::json::Value Serialize(const kafka::MessagePolled& message,
                               formats::serialize::To<formats::json::Value>) {
  formats::json::ValueBuilder builder{formats::common::Type::kObject};
  builder["topic"] = message.topic;
  builder["key"] = message.key;
  builder["payload"] = message.payload;

  return builder.ExtractValue();
}

struct RequestMessage {
  std::string topic;
  std::string key;
  std::string payload;
};

RequestMessage Parse(const formats::json::Value& doc,
                     formats::parse::To<RequestMessage>) {
  RequestMessage request_message;
  request_message.topic = doc["topic"].As<std::string>();
  request_message.key = doc["key"].As<std::string>();
  request_message.payload = doc["payload"].As<std::string>();

  return request_message;
}

bool IsCorrectRequest(const formats::json::Value& request_json) {
  const auto check_message = [](const formats::json::Value& value) {
    return value.HasMember(kReqPayloadFieldName) &&
           value.HasMember(kReqTopicFieldName) &&
           value.HasMember(kReqKeyFieldName) &&
           value.HasMember(kReqPayloadFieldName);
  };

  return check_message(request_json);
}

}  // namespace

HandlerKafkaConsumer::HandlerKafkaConsumer(
    const components::ComponentConfig& config,
    const components::ComponentContext& context)
    : server::handlers::HttpHandlerJsonBase(config, context),
      consumer_(
          context.FindComponent<kafka::ConsumerComponent>("kafka-consumer")
              .GetConsumer()
              .MakeConsumerScope()) {
  consumer_.Start([this](std::vector<kafka::MessagePolled>&& messages) {
    Consume(std::move(messages));
    consumer_.AsyncCommit();
  });
}

formats::json::Value HandlerKafkaConsumer::HandleRequestJsonThrow(
    [[maybe_unused]] const server::http::HttpRequest& request,
    [[maybe_unused]] const formats::json::Value& request_json,
    [[maybe_unused]] server::request::RequestContext& context) const {
  formats::json::ValueBuilder builder{formats::common::Type::kObject};
  builder["messages"] = ReleaseMessages();

  return builder.ExtractValue();
}

std::vector<formats::json::Value> HandlerKafkaConsumer::ReleaseMessages()
    const {
  auto thisMessages = messages_.Lock();

  std::vector<formats::json::Value> consumed_messages;
  thisMessages->swap(consumed_messages);

  return consumed_messages;
}

void HandlerKafkaConsumer::Consume(
    std::vector<kafka::MessagePolled>&& messages) {
  auto thisMessages = messages_.Lock();

  for (const auto& message : messages) {
    thisMessages->emplace_back(
        Serialize(message, formats::serialize::To<formats::json::Value>{}));
  }
}

HandlerKafkaProducers::HandlerKafkaProducers(
    const components::ComponentConfig& config,
    const components::ComponentContext& context)
    : server::handlers::HttpHandlerJsonBase(config, context) {
  for (const auto& producer_component_name : config[kProducersListFieldName]) {
    const auto parsed_producer_component_name =
        producer_component_name.As<std::string>();
    producer_by_topic_.emplace(parsed_producer_component_name,
                               context
                                   .FindComponent<kafka::ProducerComponent>(
                                       parsed_producer_component_name)
                                   .GetProducer());
  }
}

formats::json::Value HandlerKafkaProducers::HandleRequestJsonThrow(
    const server::http::HttpRequest& request,
    const formats::json::Value& request_json,
    [[maybe_unused]] server::request::RequestContext& context) const {
  if (!IsCorrectRequest(request_json)) {
    request.SetResponseStatus(server::http::HttpStatus::kBadRequest);

    return formats::json::FromString(kErrorMembersNotSet);
  }
  LOG_INFO() << "RECEIVED CORRECT REQUEST";

  const auto parse_producer =
      [this,
       &request](const formats::json::Value& message) -> kafka::Producer* {
    const auto producer_name = message[kReqProducerFieldName].As<std::string>();
    auto producer_it = producer_by_topic_.find(producer_name);
    if (producer_it == producer_by_topic_.end()) {
      request.SetResponseStatus(server::http::HttpStatus::kNotFound);

      return nullptr;
    }

    return &producer_it->second;
  };
  try {
    // TODO: add `const` after new producer implementation
    auto* producer = parse_producer(request_json);
    if (!producer) {
      return formats::json::FromString(kErrorProducerNotFound);
    }
    LOG_INFO() << "Producer found!";

    const auto message = request_json.As<RequestMessage>();
    LOG_INFO() << "Has message to " << message.topic;

    producer->Send(message.topic, message.key, message.payload);

    return formats::json::FromString(kMessageSend);
  } catch (const std::runtime_error& ex) {
    // Kafka driver does not have an API for native Kafka library error
    // handling, though it is not possible to separate client errors from
    // internal ones. Responding with bad request here gives client a chance
    // to retry the request
    request.SetResponseStatus(server::http::HttpStatus::kBadRequest);

    formats::json::ValueBuilder builder{formats::common::Type::kObject};
    builder["error"] = fmt::format("Kafka error: {}", ex.what());

    return builder.ExtractValue();
  } catch (...) {
    LOG_ERROR() << "Caught unknown exception when producing";
    request.SetResponseStatus(server::http::HttpStatus::kInternalServerError);

    return formats::json::FromString(kErrorUnknown);
  }

  return formats::json::FromString(kMessageSend);
}

yaml_config::Schema HandlerKafkaProducers::GetStaticConfigSchema() {
  return yaml_config::MergeSchemas<server::handlers::HttpHandlerJsonBase>(R"(
type: object
description: Handler of Kafka producers
additionalProperties: false
properties:
    producers_list:
        type: array
        description: TODO
        items:
          type: string
          description: TODO
)");
}

}  // namespace functional_tests

int main(int argc, char* argv[]) {
  const auto components_list =
      components::MinimalServerComponentList()
          .Append<kafka::ConsumerComponent>("kafka-consumer")
          .Append<kafka::ProducerComponent>("kafka-producer-first")
          .Append<kafka::ProducerComponent>("kafka-producer-second")
          .Append<components::TestsuiteSupport>()
          .Append<components::Secdist>()
          .Append<components::DefaultSecdistProvider>()
          .Append<components::HttpClient>()
          .Append<clients::dns::Component>()
          .Append<server::handlers::TestsControl>()
          .Append<functional_tests::HandlerKafkaConsumer>()
          .Append<functional_tests::HandlerKafkaProducers>();

  return utils::DaemonMain(argc, argv, components_list);
}
