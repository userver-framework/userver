#pragma once

#include <chrono>

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

  /// @brief Creates the Kafka Producer.
  /// @note Currently, if @param is_testsuite_mode set to `true` no Kafka
  /// interaction actually occurs. All sent messages are passed into testpoint.
  Producer(std::unique_ptr<Configuration> configuration,
           const std::string& component_name,
           engine::TaskProcessor& producer_task_processor,
           std::chrono::milliseconds poll_timeout, bool is_testsuite_mode,
           utils::statistics::Storage& storage);

  /// @brief Waits until all messages are sent for a certain timeout and destroy
  /// the inner producer.
  ~Producer();

  Producer(const Producer&) = delete;
  Producer(Producer&&) = delete;

  Producer& operator=(const Producer&) = delete;
  Producer& operator=(Producer&&) = delete;

  /// @brief Sends given message to topic @param topic_name by given @param key
  /// and `partition` (if passed) with payload contains the @param message data.
  /// Asynchronously waits until the message is delivered or the delivery error
  /// occured.
  /// @note No payload data is copied. Method holds the data until message is
  /// delivered
  /// @note thread-safe and can be called from any number of threads
  /// simultaneously
  void Send(const std::string& topic_name, std::string_view key,
            std::string_view message,
            std::optional<int> partition = std::nullopt) const;

  /// @brief Same as `void Producer::Send`, but returns the task which can be
  /// used to wait the message delivery.
  [[nodiscard]] engine::TaskWithResult<void> SendAsync(
      std::string topic_name, std::string key, std::string message,
      std::optional<int> partition = std::nullopt) const;

 private:
  /// @note For some reason creating producer in contructor breaks everything,
  /// though it should be created only on first send request
  void InitProducerAndStartPollingIfFirstSend() const;

  bool NeedSendMessageToKafka(std::string_view topic_name, std::string_view key,
                              std::string_view message) const;

  void VerifyNotFinished() const;

  /// for testsuite
  void SendToTestPoint(std::string_view topic_name, std::string_view key,
                       std::string_view message) const;

 private:
  const std::string& component_name_;
  engine::TaskProcessor& producer_task_processor_;

  const logging::LogExtra log_tags_;

  std::unique_ptr<Configuration> configuration_;

  const std::chrono::milliseconds poll_timeout_;
  mutable std::atomic<bool> first_send_{true};
  mutable std::unique_ptr<ProducerImpl>
      producer_;                    // mutable to be created on first send
  mutable engine::Task poll_task_;  // mutable to be created on first send

  /// for testsuite
  const bool is_testsuite_mode_;

  utils::statistics::Storage& storage_;
  /// @note Subscriptions must be the last fields! Add new fields above this
  /// comment.
  utils::statistics::Entry statistics_holder_;
};

}  // namespace kafka

USERVER_NAMESPACE_END
