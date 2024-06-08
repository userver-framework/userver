#include <algorithm>
#include <sstream>
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
#include <userver/kafka/components/producer_component.hpp>

#include <userver/testsuite/testsuite_support.hpp>

#include <userver/utils/daemon_run.hpp>
#include <userver/yaml_config/merge_schemas.hpp>

#include <fmt/format.h>

namespace functional_tests {

constexpr auto kProducersListFieldName = "producers_list";

constexpr std::string_view kReqProducerFieldName = "producer";
constexpr std::string_view kReqTopicFieldName = "topic";
constexpr std::string_view kReqKeyFieldName = "key";
constexpr std::string_view kReqPayloadFieldName = "payload";

constexpr std::string_view kReqTopicArgName = "topic_name";

constexpr std::string_view kMessageSend = R"(
  {
    "message": "Message send successfully"
  }
)";
constexpr std::string_view kMessagesSend = R"(
  {
    "message": "Messages send successfully"
  }
)";
constexpr std::string_view kErrorUnknown = R"(
  {
    "error": "Internal error"
  }
)";
constexpr std::string_view kErrorMembersNotSet = R"(
  {
    "error": "Expected body has 'producer', `topic`, `key` and `payload` fields"
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
  using MessagesByTopic =
      std::unordered_map<std::string, std::vector<formats::json::Value>>;

  std::vector<formats::json::Value> ReleaseMessages(
      const std::string& topic) const;

  void Consume(kafka::MessageBatchView messages);

  void DumpCurrentConsumed(
      const MessagesByTopic& messages_by_topic,
      const std::optional<std::string>& topic = std::nullopt) const;

 private:
  mutable concurrent::Variable<MessagesByTopic> messages_by_topic_;

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
  const kafka::Producer* ParseProducer(
      const formats::json::Value& request_json) const;

  formats::json::Value HandleSingleProducerRequest(
      const formats::json::Value& request_json,
      const server::http::HttpRequest& request) const;

  formats::json::Value HandleMultiProducersRequest(
      const formats::json::Value& request_json,
      const server::http::HttpRequest& request) const;

 private:
  std::unordered_map<std::string, kafka::Producer&> producer_by_topic_;
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

  if (!request_json.IsArray()) {
    return check_message(request_json);
  }

  return std::all_of(request_json.begin(), request_json.end(), check_message);
}

}  // namespace

/// [Kafka service sample - consumer usage]
HandlerKafkaConsumer::HandlerKafkaConsumer(
    const components::ComponentConfig& config,
    const components::ComponentContext& context)
    : server::handlers::HttpHandlerJsonBase(config, context),
      consumer_(
          context.FindComponent<kafka::ConsumerComponent>("kafka-consumer")
              .GetConsumer()) {
  consumer_.Start([this](kafka::MessageBatchView messages) {
    Consume(messages);
    consumer_.AsyncCommit();
  });
}
/// [Kafka service sample - consumer usage]

formats::json::Value HandlerKafkaConsumer::HandleRequestJsonThrow(
    const server::http::HttpRequest& request,
    [[maybe_unused]] const formats::json::Value& request_json,
    [[maybe_unused]] server::request::RequestContext& context) const {
  const auto& topic = request.GetPathArg(kReqTopicArgName);

  formats::json::ValueBuilder builder{formats::common::Type::kObject};
  builder["messages"] = ReleaseMessages(topic);

  return builder.ExtractValue();
}

std::vector<formats::json::Value> HandlerKafkaConsumer::ReleaseMessages(
    const std::string& topic) const {
  auto thisMessages = messages_by_topic_.Lock();

  if (topic.empty()) {
    LOG_WARNING() << "Consuming messages from all topics!";

    std::vector<formats::json::Value> consumed_messages;
    for (auto&& topic_messages : *thisMessages) {
      LOG_WARNING() << "Clearing topic: " << topic_messages.first;
      auto& messages = topic_messages.second;
      consumed_messages.reserve(consumed_messages.size() + messages.size());
      std::move(std::make_move_iterator(messages.begin()),
                std::make_move_iterator(messages.end()),
                std::back_inserter(consumed_messages));
      messages.clear();
    }

    thisMessages->clear();

    return consumed_messages;
  }

  const auto topic_messages_it = thisMessages->find(topic);
  if (topic_messages_it == thisMessages->end()) {
    return {};
  }

  DumpCurrentConsumed(*thisMessages, topic);

  std::vector<formats::json::Value> consumed_messages;
  topic_messages_it->second.swap(consumed_messages);

  return consumed_messages;
}

void HandlerKafkaConsumer::Consume(kafka::MessageBatchView messages) {
  auto thisMessages = messages_by_topic_.Lock();

  for (const auto& message : messages) {
    if (!message.GetTimestamp().has_value()) {
      continue;
    }

    (*thisMessages)[message.GetTopic()].emplace_back(
        Serialize(message, formats::serialize::To<formats::json::Value>{}));
  }

  DumpCurrentConsumed(*thisMessages);
}

void HandlerKafkaConsumer::DumpCurrentConsumed(
    const MessagesByTopic& messages_by_topic,
    const std::optional<std::string>& topic) const {
  LOG_DEBUG() << fmt::format("Messages of {}:\n", topic.value_or("all topics"));

  const auto format_topic_messages =
      [](const std::string& topic,
         const MessagesByTopic::mapped_type& messages) {
        std::stringstream ss;

        ss << fmt::format("Topic '{}' messages:", topic);
        for (const auto& message : messages) {
          ss << fmt::format("Message: {}", message);
        }

        return ss.str();
      };
  if (!logging::ShouldLog(logging::Level::kDebug)) {
    return;
  }

  if (topic.has_value()) {
    LOG_DEBUG() << format_topic_messages(topic.value(),
                                         messages_by_topic.at(topic.value()));
    return;
  }
  for (const auto& topic : messages_by_topic) {
    LOG_DEBUG() << format_topic_messages(topic.first, topic.second);
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

  try {
    if (!request_json.IsArray()) {
      return HandleSingleProducerRequest(request_json, request);
    } else {
      return HandleMultiProducersRequest(request_json, request);
    }
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
        description: list of producer names
        items:
          type: string
          description: producer name
)");
}

const kafka::Producer* HandlerKafkaProducers::ParseProducer(
    const formats::json::Value& request_json) const {
  const auto producer_name =
      request_json[kReqProducerFieldName].As<std::string>();
  auto producer_it = producer_by_topic_.find(producer_name);
  if (producer_it == producer_by_topic_.end()) {
    return nullptr;
  }

  return &producer_it->second;
}

formats::json::Value HandlerKafkaProducers::HandleSingleProducerRequest(
    const formats::json::Value& request_json,
    const server::http::HttpRequest& request) const {
  const auto* producer = ParseProducer(request_json);
  if (!producer) {
    request.SetResponseStatus(server::http::HttpStatus::kBadRequest);

    return formats::json::FromString(kErrorProducerNotFound);
  }

  const auto message = request_json.As<RequestMessage>();

  producer->Send(message.topic, message.key, message.payload);

  return formats::json::FromString(kMessageSend);
}

formats::json::Value HandlerKafkaProducers::HandleMultiProducersRequest(
    const formats::json::Value& request_json,
    const server::http::HttpRequest& request) const {
  std::vector<const kafka::Producer*> producers;
  producers.reserve(request_json.GetSize());
  for (const auto& request_message : request_json) {
    const auto* producer = ParseProducer(request_message);
    if (!producer) {
      request.SetResponseStatus(server::http::HttpStatus::kBadRequest);

      return formats::json::FromString(kErrorProducerNotFound);
    }

    producers.push_back(producer);
  }

  std::vector<engine::TaskWithResult<void>> send_tasks;
  send_tasks.reserve(producers.size());
  for (std::size_t i = 0; i < producers.size(); ++i) {
    const auto* producer = producers.at(i);
    auto message = request_json[i].As<RequestMessage>();

    send_tasks.emplace_back(producer->SendAsync(std::move(message.topic),
                                                std::move(message.key),
                                                std::move(message.payload)));
  }

  engine::WaitAllChecked(send_tasks);

  return formats::json::FromString(kMessagesSend);
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
