#pragma once

#include <chrono>
#include <cstdint>

#include <userver/engine/task/task.hpp>
#include <userver/engine/task/task_processor_fwd.hpp>
#include <userver/engine/task/task_with_result.hpp>
#include <userver/utils/statistics/writer.hpp>

USERVER_NAMESPACE_BEGIN

namespace kafka {

namespace impl {

class Configuration;
class ProducerImpl;

struct ProducerConfiguration;
struct Secret;

}  // namespace impl

/// @brief Parameters `Producer` uses in runtime.
/// The struct is used only for documentation purposes, `Producer` can be
/// created through `ProducerComponent`.
struct ProducerExecutionParams final {
  /// @brief Time producer waits for new delivery events.
  std::chrono::milliseconds poll_timeout{10};

  /// @brief How many times `Produce::Send*` retries when delivery
  /// failures. Retries take place only when errors are transient.
  ///
  /// @remark `librdkafka` already has a retry mechanism. Moreover, user-retried
  /// requests may lead to messages reordering or duplication. Nevertheless, the
  /// library retries a small list of delivery errors (such as message
  /// guaranteed timeouts), including errors those are not retried by
  /// `librdkafka` and errors that may occur when the Kafka cluster or topic
  /// have just been created (for instance, in tests)
  /// @see impl/producer_impl.cpp for the list of errors retryable by library
  std::size_t send_retries{5};
};

/// @ingroup userver_clients
///
/// @brief Apache Kafka Producer Client.
///
/// It exposes the API to send an arbitrary message to the Kafka Broker to
/// certain topic by given key and optional partition.
///
/// ## Important implementation details
///
/// Implementation does not block on any send to Kafka and asynchronously
/// waits for each message to be delivered.
///
/// `Producer` periodically polls the metadata about delivered messages
/// from Kafka Broker, blocking for some time, in separate task processor.
///
/// `Producer` maintains the per topic statistics including the broker
/// connection errors.
///
/// @remark Destructor may block for no more than a couple of seconds to ensure
/// all sent messages are properly delivered
///
/// @see https://docs.confluent.io/platform/current/clients/producer.html
class Producer final {
 public:
  /// @brief Creates the Kafka Producer.
  ///
  /// @param producer_task_processor where producer polls for delivery reports
  /// and creates tasks for message delivery scheduling.
  /// Currently, producer_task_processor **must contain at least 2
  /// threads for each producer**
  Producer(const std::string& name,
           engine::TaskProcessor& producer_task_processor,
           const impl::ProducerConfiguration& configuration,
           const impl::Secret& secrets, ProducerExecutionParams params);

  /// @brief Waits until all messages are sent for a certain timeout and destroy
  /// the inner producer.
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
  /// thread-safe and can be called from any number of threads
  /// simultaneously.
  ///
  /// `Producer::Send` call may take at most
  /// `delivery_timeout` x `send_retries_count` milliseconds.
  ///
  /// If `partition` not passed, partition is chosen by internal
  /// Kafka partitioner.
  ///
  /// @warning if `enable_idempotence` option is enabled, do not use both
  /// explicit partitions and Kafka-chosen ones
  /// @throws std::runtime_error if message is not delivery and acked by Kafka
  /// Broker
  void Send(const std::string& topic_name, std::string_view key,
            std::string_view message,
            std::optional<std::uint32_t> partition = std::nullopt) const;

  /// @brief Same as `Producer::Send`, but returns the task which can be
  /// used to wait the message delivery.
  ///
  /// @warning If user schedules a batch of send requests with
  /// `Producer::SendAsync`, some send
  /// requests may be retried by the library (for instance, in case of network
  /// blink). Though, the order messages are written to partition may differ
  /// from the order messages are initially sent
  [[nodiscard]] engine::TaskWithResult<void> SendAsync(
      std::string topic_name, std::string key, std::string message,
      std::optional<std::uint32_t> partition = std::nullopt) const;

  /// @brief Dumps per topic messages produce statistics.
  /// @see impl/stats.hpp
  void DumpMetric(utils::statistics::Writer& writer) const;

 private:
  void InitProducerAndStartPollingIfFirstSend() const;

  void VerifyNotFinished() const;

  /// @note for testsuite
  void SendToTestPoint(std::string_view topic_name, std::string_view key,
                       std::string_view message) const;

  /// @brief Adds consumer name to current span.
  void ExtendCurrentSpan() const;

 private:
  const std::string component_name_;
  engine::TaskProcessor& producer_task_processor_;

  const std::chrono::milliseconds poll_timeout_{};
  const std::size_t send_retries_{};

  mutable std::atomic<bool> first_send_{true};
  mutable std::unique_ptr<impl::Configuration> configuration_;
  mutable std::unique_ptr<impl::ProducerImpl>
      producer_;                    // mutable to be created on first send
  mutable engine::Task poll_task_;  // mutable to be created on first send
};

}  // namespace kafka

USERVER_NAMESPACE_END
