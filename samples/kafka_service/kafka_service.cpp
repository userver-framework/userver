#include <userver/utest/using_namespace_userver.hpp>

#include <fmt/format.h>

#include <userver/kafka/consumer_component.hpp>
#include <userver/kafka/producer_component.hpp>

#include <userver/components/component_base.hpp>
#include <userver/components/component_config.hpp>
#include <userver/components/component_context.hpp>
#include <userver/components/minimal_server_component_list.hpp>

#include <userver/clients/dns/component.hpp>
#include <userver/clients/http/component.hpp>
#include <userver/formats/json/value_builder.hpp>

#include <userver/server/handlers/http_handler_json_base.hpp>
#include <userver/server/handlers/tests_control.hpp>

#include <userver/storages/secdist/component.hpp>
#include <userver/storages/secdist/provider_component.hpp>

#include <userver/testsuite/testpoint.hpp>
#include <userver/testsuite/testsuite_support.hpp>

#include <userver/utils/daemon_run.hpp>

namespace samples {

constexpr std::string_view kReqTopicFieldName = "topic";
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
    "error": "Expected body has `topic`, `key` and `payload` fields"
  }
)";

class HandlerKafkaConsumer final : public components::ComponentBase {
 public:
  static constexpr std::string_view kName{"handler-kafka-consumer"};

  HandlerKafkaConsumer(const components::ComponentConfig& config,
                       const components::ComponentContext& context);

 private:
  void Consume(kafka::MessageBatchView messages);

 private:
  // Subscriptions must be the last fields! Add new fields above this comment.
  kafka::ConsumerScope consumer_;
};

class HandlerKafkaProducer final
    : public server::handlers::HttpHandlerJsonBase {
 public:
  static constexpr std::string_view kName{"handler-kafka-producer"};

  HandlerKafkaProducer(const components::ComponentConfig& config,
                       const components::ComponentContext& context);

  formats::json::Value HandleRequestJsonThrow(
      const server::http::HttpRequest& request,
      const formats::json::Value& request_json,
      server::request::RequestContext& context) const override;

 private:
  kafka::Producer& producer_;
};

}  // namespace samples

namespace samples {

namespace {

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
  return request_json.HasMember(kReqPayloadFieldName) &&
         request_json.HasMember(kReqTopicFieldName) &&
         request_json.HasMember(kReqKeyFieldName);
}

}  // namespace

HandlerKafkaConsumer::HandlerKafkaConsumer(
    const components::ComponentConfig& config,
    const components::ComponentContext& context)
    : components::LoggableComponentBase(config, context),
      consumer_(
          context.FindComponent<kafka::ConsumerComponent>("kafka-consumer")
              .GetConsumer()) {
  consumer_.Start([this](kafka::MessageBatchView messages) {
    Consume(messages);
    consumer_.AsyncCommit();
  });
}

void HandlerKafkaConsumer::Consume(kafka::MessageBatchView messages) {
  for (const auto& message : messages) {
    if (!message.GetTimestamp().has_value()) {
      continue;
    }

    TESTPOINT("message_consumed", [&message] {
      formats::json::ValueBuilder builder;
      builder["key"] = message.GetKey();

      return builder.ExtractValue();
    }());
  }
}

HandlerKafkaProducer::HandlerKafkaProducer(
    const components::ComponentConfig& config,
    const components::ComponentContext& context)
    : server::handlers::HttpHandlerJsonBase(config, context),
      producer_(
          context.FindComponent<kafka::ProducerComponent>("kafka-producer")
              .GetProducer()) {}

formats::json::Value HandlerKafkaProducer::HandleRequestJsonThrow(
    const server::http::HttpRequest& request,
    const formats::json::Value& request_json,
    [[maybe_unused]] server::request::RequestContext& context) const {
  if (!IsCorrectRequest(request_json)) {
    request.SetResponseStatus(server::http::HttpStatus::kBadRequest);

    return formats::json::FromString(kErrorMembersNotSet);
  }

  const auto message = request_json.As<RequestMessage>();

  try {
    producer_.Send(message.topic, message.key, message.payload);

    return formats::json::FromString(kMessageSend);
  } catch (const kafka::SendException& ex) {
    request.SetResponseStatus(ex.IsRetryable()
                                  ? server::http::HttpStatus::TooManyRequests
                                  : server::http::HttpStatus::kBadRequest);

    formats::json::ValueBuilder builder{formats::common::Type::kObject};
    builder["error"] = fmt::format("Kafka error: {}", ex.what());

    return builder.ExtractValue();
  } catch (const std::runtime_error& ex) {
    request.SetResponseStatus(server::http::HttpStatus::kInternalServerError);

    formats::json::ValueBuilder builder{formats::common::Type::kObject};
    builder["error"] = fmt::format("Kafka error: {}", ex.what());

    return builder.ExtractValue();
  } catch (...) {
    LOG_ERROR() << "Caught unknown exception when producing";
    request.SetResponseStatus(server::http::HttpStatus::kInternalServerError);

    return formats::json::FromString(kErrorUnknown);
  }
}

}  // namespace samples

int main(int argc, char* argv[]) {
  const auto components_list =
      components::MinimalServerComponentList()
          .Append<kafka::ConsumerComponent>("kafka-consumer")
          .Append<kafka::ProducerComponent>("kafka-producer")
          .Append<components::TestsuiteSupport>()
          .Append<components::Secdist>()
          .Append<components::DefaultSecdistProvider>()
          .Append<components::HttpClient>()
          .Append<clients::dns::Component>()
          .Append<server::handlers::TestsControl>()
          .Append<samples::HandlerKafkaConsumer>()
          .Append<samples::HandlerKafkaProducer>();

  return utils::DaemonMain(argc, argv, components_list);
}
