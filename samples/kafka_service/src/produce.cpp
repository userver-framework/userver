#include <produce.hpp>

#include <userver/kafka/exceptions.hpp>

namespace kafka_sample {

/// [Kafka service sample - produce]
SendStatus Produce(const kafka::Producer& producer, const RequestMessage& message) {
    try {
        producer.Send(message.topic, message.key, message.payload);
        return SendStatus::kSuccess;
    } catch (const kafka::SendException& ex) {
        return ex.IsRetryable() ? SendStatus::kErrorRetryable : SendStatus::kErrorNonRetryable;
    }
}
/// [Kafka service sample - produce]

}  // namespace kafka_sample
