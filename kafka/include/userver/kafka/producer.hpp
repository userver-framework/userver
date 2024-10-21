#pragma once

#include <cstdint>

#include <userver/engine/task/task_processor_fwd.hpp>
#include <userver/engine/task/task_with_result.hpp>
#include <userver/kafka/exceptions.hpp>
#include <userver/utils/fast_pimpl.hpp>
#include <userver/utils/statistics/writer.hpp>

USERVER_NAMESPACE_BEGIN

namespace kafka {

namespace impl {

class Configuration;
class ProducerImpl;

struct ProducerConfiguration;
struct Secret;

}  // namespace impl

/// @ingroup userver_clients
///
/// @brief Apache Kafka Producer Client.
///
/// It exposes the API to send an arbitrary message to the Kafka Broker to
/// certain topic by given key and optional partition.
///
/// ## Important implementation details
///
/// Message send tasks handling other messages' delivery reports, suspending
/// their execution while no events exist.
/// This makes message production parallel and leads to high Producer
/// scalability.
///
/// Producer maintains per topic statistics including the broker
/// connection errors.
///
/// @remark Destructor may wait for no more than a 2 x `delivery_timeout` to
/// ensure all sent messages are properly delivered.
///
/// @see https://docs.confluent.io/platform/current/clients/producer.html
class Producer final {
public:
    /// @brief Creates the Kafka Producer.
    ///
    /// @param producer_task_processor is task processor where producer creates
    /// tasks for message delivery scheduling and waiting.
    Producer(
        const std::string& name,
        engine::TaskProcessor& producer_task_processor,
        const impl::ProducerConfiguration& configuration,
        const impl::Secret& secrets
    );

    /// @brief Waits until all messages are sent for at most 2 x
    /// `delivery_timeout` and destroys the producer.
    ///
    /// @remark In a basic producer use cases, the destructor returns immediately.
    ~Producer();

    Producer(const Producer&) = delete;
    Producer(Producer&&) = delete;

    Producer& operator=(const Producer&) = delete;
    Producer& operator=(Producer&&) = delete;

    /// @brief Sends given message to topic `topic_name` by given `key`
    /// and `partition` (if passed) with payload contains the `message`
    /// data. Asynchronously waits until the message is delivered or the delivery
    /// error occurred.
    ///
    /// No payload data is copied. Method holds the data until message is
    /// delivered.
    ///
    /// Thread-safe and can be called from any number of threads
    /// concurrently.
    ///
    /// If `partition` not passed, partition is chosen by internal
    /// Kafka partitioner.
    ///
    /// @warning if `enable_idempotence` option is enabled, do not use both
    /// explicit partitions and Kafka-chosen ones.
    ///
    /// @throws SendException and its descendants if message is not delivered
    /// and acked by Kafka Broker in configured timeout.
    ///
    /// @note Use SendException::IsRetryable method to understand whether there is
    /// a sense to retry the message sending.
    /// @snippet kafka/tests/producer_kafkatest.cpp Producer retryable error
    void Send(
        const std::string& topic_name,
        std::string_view key,
        std::string_view message,
        std::optional<std::uint32_t> partition = std::nullopt
    ) const;

    /// @brief Same as Producer::Send, but returns the task which can be
    /// used to wait the message delivery manually.
    ///
    /// @warning If user schedules a batch of send requests with
    /// Producer::SendAsync, some send
    /// requests may be retried by the library (for instance, in case of network
    /// blink). Though, the order messages are written to partition may differ
    /// from the order messages are initially sent
    /// @snippet kafka/tests/producer_kafkatest.cpp Producer batch send async
    [[nodiscard]] engine::TaskWithResult<void> SendAsync(
        std::string topic_name,
        std::string key,
        std::string message,
        std::optional<std::uint32_t> partition = std::nullopt
    ) const;

    /// @brief Dumps per topic messages produce statistics. No expected to be
    /// called manually.
    /// @see kafka/impl/stats.hpp
    void DumpMetric(utils::statistics::Writer& writer) const;

private:
    void SendImpl(
        const std::string& topic_name,
        std::string_view key,
        std::string_view message,
        std::optional<std::uint32_t> partition
    ) const;

private:
    const std::string name_;
    engine::TaskProcessor& producer_task_processor_;

    static constexpr std::size_t kImplSize{944};
    static constexpr std::size_t kImplAlign{16};
    utils::FastPimpl<impl::ProducerImpl, kImplSize, kImplAlign> producer_;
};

}  // namespace kafka

USERVER_NAMESPACE_END
