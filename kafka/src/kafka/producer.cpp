#include <userver/kafka/producer.hpp>

#include <userver/formats/json/value_builder.hpp>
#include <userver/kafka/impl/configuration.hpp>
#include <userver/kafka/impl/stats.hpp>
#include <userver/testsuite/testpoint.hpp>
#include <userver/tracing/span.hpp>
#include <userver/utils/async.hpp>
#include <userver/utils/text_light.hpp>

#include <kafka/impl/producer_impl.hpp>

USERVER_NAMESPACE_BEGIN

namespace kafka {

namespace {

void SendToTestPoint(std::string_view component_name,
                     std::string_view topic_name, std::string_view key,
                     std::string_view message,
                     std::optional<std::uint32_t> partition) {
  // Testpoint server does not accept non-utf8 data
  TESTPOINT(fmt::format("tp_{}", component_name), [&] {
    formats::json::ValueBuilder builder;
    builder["topic"] = topic_name;
    builder["key"] = key;
    if (utils::text::IsUtf8(message)) {
      builder["message"] = message;
    }
    if (partition.has_value()) {
      builder["partition"] = partition.value();
    }
    return builder.ExtractValue();
  }());
}

[[noreturn]] void ThrowSendError(const impl::DeliveryResult& delivery_result) {
  const auto error = delivery_result.GetMessageError();
  UASSERT(error != RD_KAFKA_RESP_ERR_NO_ERROR);

  switch (error) {
    case RD_KAFKA_RESP_ERR__MSG_TIMED_OUT:
      throw DeliveryTimeoutException{};
    case RD_KAFKA_RESP_ERR__QUEUE_FULL:
      throw QueueFullException{};
    case RD_KAFKA_RESP_ERR_MSG_SIZE_TOO_LARGE:
      throw MessageTooLargeException{};
    case RD_KAFKA_RESP_ERR__UNKNOWN_TOPIC:
      throw kafka::UnknownTopicException{};
    case RD_KAFKA_RESP_ERR__UNKNOWN_PARTITION:
      throw kafka::UnknownPartitionException{};
    default:
      throw kafka::SendException{rd_kafka_err2str(error)};
  }

  UASSERT(false);
}

}  // namespace

Producer::Producer(const std::string& name,
                   engine::TaskProcessor& producer_task_processor,
                   const impl::ProducerConfiguration& configuration,
                   const impl::Secret& secrets)
    : name_(name),
      producer_task_processor_(producer_task_processor),
      producer_(impl::Configuration{name, configuration, secrets}) {}

Producer::~Producer() {
  utils::Async(producer_task_processor_, "producer_shutdown", [this] {
    std::move(*producer_).WaitUntilAllMessagesDelivered();
  }).Get();
}

void Producer::Send(const std::string& topic_name, std::string_view key,
                    std::string_view message,
                    std::optional<std::uint32_t> partition) const {
  utils::Async(producer_task_processor_, "producer_send",
               [this, &topic_name, key, message, partition] {
                 SendImpl(topic_name, key, message, partition);
               })
      .Get();
}

engine::TaskWithResult<void> Producer::SendAsync(
    std::string topic_name, std::string key, std::string message,
    std::optional<std::uint32_t> partition) const {
  return utils::Async(
      producer_task_processor_, "producer_send_async",
      [this, topic_name = std::move(topic_name), key = std::move(key),
       message = std::move(message),
       partition] { SendImpl(topic_name, key, message, partition); });
}

void Producer::DumpMetric(utils::statistics::Writer& writer) const {
  impl::DumpMetric(writer, producer_->GetStats());
}

void Producer::SendImpl(const std::string& topic_name, std::string_view key,
                        std::string_view message,
                        std::optional<std::uint32_t> partition) const {
  tracing::Span::CurrentSpan().AddTag("kafka_producer", name_);

  const impl::DeliveryResult delivery_result =
      producer_->Send(topic_name, key, message, partition);
  if (!delivery_result.IsSuccess()) {
    ThrowSendError(delivery_result);
  }

  SendToTestPoint(name_, topic_name, key, message, partition);
}

}  // namespace kafka

USERVER_NAMESPACE_END
