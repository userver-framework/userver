#pragma once

#include <chrono>
#include <cstdint>
#include <optional>

#include <librdkafka/rdkafka.h>

#include <kafka/impl/delivery_waiter.hpp>
#include <kafka/impl/holders.hpp>
#include <kafka/impl/stats.hpp>

USERVER_NAMESPACE_BEGIN

namespace kafka::impl {

class Configuration;

class ProducerImpl final {
  static constexpr std::chrono::milliseconds kCoolDownFlushTimeout{2000};

 public:
  explicit ProducerImpl(Configuration&& configuration);

  ~ProducerImpl();

  /// @brief Send the message and waits for its delivery.
  void Send(const std::string& topic_name, std::string_view key,
            std::string_view message, std::optional<std::uint32_t> partition,
            std::uint32_t max_retries) const;

  /// @brief Polls for delivery events for `poll_timeout_` milliseconds
  void Poll(std::chrono::milliseconds poll_timeout) const;

  const Stats& GetStats() const;

  void ErrorCallback(int error_code, const char* reason);

  /// @brief Callback called on each succeeded/failed message delivery.
  /// @param message represents the delivered (or not) message. Its `_private`
  /// field contains for `opaque` argument, which was passed to
  /// `rd_kafka_producev`
  void DeliveryReportCallback(const rd_kafka_message_s* message);

 private:
  DeliveryResult SendImpl(const std::string& topic_name, std::string_view key,
                          std::string_view message,
                          std::optional<std::uint32_t> partition,
                          std::uint32_t current_retry,
                          std::uint32_t max_retries) const;

 private:
  Stats stats_;
  ProducerHolder producer_;
};

}  // namespace kafka::impl

USERVER_NAMESPACE_END
