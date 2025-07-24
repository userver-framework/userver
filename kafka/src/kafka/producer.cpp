#include <userver/kafka/producer.hpp>

#include <userver/formats/json/value_builder.hpp>
#include <userver/formats/serialize/common_containers.hpp>
#include <userver/kafka/impl/configuration.hpp>
#include <userver/kafka/impl/stats.hpp>
#include <userver/testsuite/testpoint.hpp>
#include <userver/tracing/span.hpp>
#include <userver/utils/async.hpp>
#include <userver/utils/text_light.hpp>

#include <kafka/impl/producer_impl.hpp>

USERVER_NAMESPACE_BEGIN

namespace kafka {

const std::optional<std::uint32_t> kUnassignedPartition{std::nullopt};

formats::json::Value Serialize(const OwningHeader& header, formats::serialize::To<formats::json::Value>) {
    return formats::json::MakeObject("name", header.GetName(), "value", std::string{header.GetValue()});
}

namespace {

constexpr std::string_view kNonUtf8MessagePlaceholder = "<non-utf8-message>";

void SendToTestPoint(
    std::string_view component_name,
    std::string_view topic_name,
    std::string_view key,
    std::string_view message,
    std::optional<std::uint32_t> partition,
    const std::vector<OwningHeader>& headers,
    const std::optional<std::string>& suffix = std::nullopt
) {
    // Testpoint server does not accept non-utf8 data
    TESTPOINT(fmt::format("tp_{}{}", component_name, suffix.value_or("")), [&] {
        formats::json::ValueBuilder builder;
        builder["topic"] = topic_name;
        builder["key"] = key;
        if (utils::text::IsUtf8(message)) {
            builder["message"] = message;
        } else {
            builder["message"] = kNonUtf8MessagePlaceholder;
        }
        if (partition.has_value()) {
            builder["partition"] = partition.value();
        }
        if (!headers.empty()) {
            builder["headers"] = headers;
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

Producer::Producer(
    const std::string& name,
    engine::TaskProcessor& producer_task_processor,
    const impl::ProducerConfiguration& configuration,
    const impl::Secret& secrets
)
    : name_(name),
      producer_task_processor_(producer_task_processor),
      producer_(
          impl::Configuration{name, configuration, secrets},
          configuration.debug_info_log_level,
          configuration.operation_log_level
      ) {}

Producer::~Producer() {
    utils::Async(producer_task_processor_, "producer_shutdown", [this] {
        std::move(*producer_).WaitUntilAllMessagesDelivered();
    }).Get();
}

void Producer::Send(
    const std::string& topic_name,
    std::string_view key,
    std::string_view message,
    std::optional<std::uint32_t> partition,
    HeaderViews headers
) const {
    utils::Async(producer_task_processor_, "producer_send", [this, &topic_name, key, message, partition, &headers] {
        SendImpl(topic_name, key, message, partition, impl::HeadersHolder{headers});
    }).Get();
}

engine::TaskWithResult<void> Producer::SendAsync(
    std::string topic_name,
    std::string key,
    std::string message,
    std::optional<std::uint32_t> partition,
    HeaderViews headers
) const {
    return utils::Async(
        producer_task_processor_,
        "producer_send_async",
        [this,
         topic_name = std::move(topic_name),
         key = std::move(key),
         message = std::move(message),
         partition,
         headers_holder = impl::HeadersHolder{headers}]() mutable {
            SendImpl(topic_name, key, message, partition, std::move(headers_holder));
        }
    );
}

void Producer::DumpMetric(utils::statistics::Writer& writer) const {
    impl::DumpMetric(writer, producer_->GetStats(), this->name_);
}

void Producer::SendImpl(
    const std::string& topic_name,
    std::string_view key,
    std::string_view message,
    std::optional<std::uint32_t> partition,
    impl::HeadersHolder&& headers_holder
) const {
    tracing::Span::CurrentSpan().AddTag("kafka_producer", name_);
    tracing::Span::CurrentSpan().AddTag("kafka_send_key", std::string{key});

    std::vector<OwningHeader> headers_copy;
    if (testsuite::AreTestpointsAvailable()) {
        auto reader = HeadersReader{headers_holder.GetHandle()};
        headers_copy = std::vector<OwningHeader>{reader.begin(), reader.end()};
    }
    if (testsuite::AreTestpointsAvailable()) {
        SendToTestPoint(name_, topic_name, key, message, partition, headers_copy, "::started");
    }

    const impl::DeliveryResult delivery_result =
        producer_->Send(topic_name, key, message, partition, std::move(headers_holder));
    if (!delivery_result.IsSuccess()) {
        ThrowSendError(delivery_result);
    }

    if (testsuite::AreTestpointsAvailable()) {
        SendToTestPoint(name_, topic_name, key, message, partition, headers_copy);
    }
}

}  // namespace kafka

USERVER_NAMESPACE_END
