#pragma once

#include <chrono>
#include <cstdint>

#include <userver/engine/task/task_with_result.hpp>
#include <userver/logging/log_extra.hpp>
#include <userver/utils/statistics/entry.hpp>
#include <userver/utils/statistics/storage.hpp>

USERVER_NAMESPACE_BEGIN

namespace kafka {

using namespace std::chrono_literals;

class Configuration;
class ProducerImpl;

/// @brief `Producer` represents the Apache Kafka Producer
/// It exposes the API to send an arbitrary message to the Kafka Broker to
/// certain topic by given key and optional partition.
/// @note Implementation does not block on any send to Kafka and asynchronously
/// waits for each message to be delivered
/// @note `Producer` periodically polls the metadata about delivered messages
/// from Kafka Broker, blocking for some time, in separate task processor
/// @note `Producer` maintains the per topic statistics including the broker
/// connection errors
/// @remark Destructor may block for no more than a couple of seconds to ensure
/// all sent messages are properly delivered
/// @see https://docs.confluent.io/platform/current/clients/producer.html
class Producer {
 public:
  /// @brief Time producer waits for new delivery events.
  static constexpr auto kDefaultPollTimeout = 10ms;

  /// @brief How many times `Produce::Send*` retries when delivery
  /// failures. Retries take place only when errors are transient.
  /// @remark `librdkafka` already has a retry mechanism. Moreover, user-retried
  /// requests may lead to messages reordering or duplication. Nevertheless, the
  /// library retries a small list of delivery errors (such as message
  /// guaranteed timeouts), including errors those are not retried by
  /// `librdkafka` and errors that may occure when the Kafka cluster or topic
  /// have just been created (for instance, in tests)
  /// @see impl/producer_impl.cpp for the list of errors retryable by library
  static constexpr std::size_t kDefaultSendRetries = 5;

  /// @brief Creates the Kafka Producer.
  Producer(std::unique_ptr<Configuration> configuration,
           const std::string& component_name,
           engine::TaskProcessor& producer_task_processor,
           std::chrono::milliseconds poll_timeout, std::size_t send_retries,
           utils::statistics::Storage& storage);

  /// @brief Waits until all messages are sent for a certain timeout and destroy
  /// the inner producer.
  ~Producer();

  Producer(const Producer&) = delete;
  Producer(Producer&&) = delete;

  Producer& operator=(const Producer&) = delete;
  Producer& operator=(Producer&&) = delete;

  /// @brief Sends given message to topic @param topic_name by given @param key
  /// and @param partition (if passed) with payload contains the @param message
  /// data. Asynchronously waits until the message is delivered or the delivery
  /// error occured.
  /// @note No payload data is copied. Method holds the data until message is
  /// delivered
  /// @note thread-safe and can be called from any number of threads
  /// simultaneously
  /// @note `Producer::Send` call may take at most
  /// `delivery_timeout_ms` x `send_retries_count` milliseconds
  /// @note If @param partition not passed, partition will be chosen by internal
  /// Kafka partitioner
  /// @warning if `enable_idempotence` option is enabled, do not use both
  /// explicit partitions and Kafka-chosen ones
  /// @throws std::runtime_error if message is not delivery and acked by Kafka
  /// broker
  void Send(const std::string& topic_name, std::string_view key,
            std::string_view message,
            std::optional<std::uint32_t> partition = std::nullopt) const;

  /// @brief Same as `Producer::Send`, but returns the task which can be
  /// used to wait the message delivery.
  /// @warning If user schedules a batch of send requests with
  /// `Producer::SendAsync`, some send
  /// requests may be retried by the library (for instance, in case of network
  /// blink). Though, the order messages are written to partition may differ
  /// from the order messages are initially sent
  [[nodiscard]] engine::TaskWithResult<void> SendAsync(
      std::string topic_name, std::string key, std::string message,
      std::optional<std::uint32_t> partition = std::nullopt) const;

 private:
  void InitProducerAndStartPollingIfFirstSend() const;

  void VerifyNotFinished() const;

  /// for testsuite
  void SendToTestPoint(std::string_view topic_name, std::string_view key,
                       std::string_view message) const;

 private:
  const std::string& component_name_;
  engine::TaskProcessor& producer_task_processor_;

  const logging::LogExtra log_tags_;

  std::unique_ptr<Configuration> configuration_;

  const std::chrono::milliseconds poll_timeout_{};
  const std::size_t send_retries_{};
  mutable std::atomic<bool> first_send_{true};
  mutable std::unique_ptr<ProducerImpl>
      producer_;                    // mutable to be created on first send
  mutable engine::Task poll_task_;  // mutable to be created on first send

  utils::statistics::Storage& storage_;
  /// @note Subscriptions must be the last fields! Add new fields above this
  /// comment.
  utils::statistics::Entry statistics_holder_;
};

}  // namespace kafka

USERVER_NAMESPACE_END
