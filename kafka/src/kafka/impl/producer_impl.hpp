#pragma once

#include <chrono>
#include <cstdint>
#include <memory>
#include <optional>

#include <kafka/impl/delivery_waiter.hpp>
#include <kafka/impl/stats.hpp>

#include <librdkafka/rdkafka.h>

USERVER_NAMESPACE_BEGIN

namespace kafka::impl {

class Configuration;

class ProducerImpl final {
 public:
  static constexpr std::chrono::milliseconds kCoolDownFlushTimeout{2000};

  explicit ProducerImpl(std::unique_ptr<Configuration> configuration);

  ~ProducerImpl();

  /// @brief Send the message and waits for its delivery.
  void Send(const std::string& topic_name, std::string_view key,
            std::string_view message, std::optional<std::uint32_t> partition,
            std::uint32_t max_retries) const;

  /// @brief Polls for delivery events for `poll_timeout_` milliseconds
  void Poll(std::chrono::milliseconds poll_timeout) const;

  const Stats& GetStats() const;

  void ErrorCallbackProxy(int error_code, const char* reason);

  /// @brief Callback called on each succeeded/failed message delivery.
  /// @param message represents the delivered (or not) message. Its `_private`
  /// field contains for `opaque` argument, which was passed to
  /// `rd_kafka_producev`
  void DeliveryReportCallbackProxy(const rd_kafka_message_s* message);

 private:
  DeliveryResult SendImpl(const std::string& topic_name, std::string_view key,
                          std::string_view message,
                          std::optional<std::uint32_t> partition,
                          std::uint32_t current_retry,
                          std::uint32_t max_retries) const;

 private:
  Stats stats_;

  class ProducerHolder final {
   public:
    ProducerHolder(rd_kafka_conf_t* conf);

    ~ProducerHolder();

    ProducerHolder(const ProducerHolder&) = delete;
    ProducerHolder& operator=(const ProducerHolder&) = delete;

    ProducerHolder(ProducerHolder&&) noexcept = delete;
    ProducerHolder& operator=(ProducerHolder&&) noexcept = delete;

    rd_kafka_t* Handle() const;

   private:
    using HandleHolder =
        std::unique_ptr<rd_kafka_t, decltype(&rd_kafka_destroy)>;
    HandleHolder handle_{nullptr, &rd_kafka_destroy};
  };
  ProducerHolder producer_;
};

}  // namespace kafka::impl

USERVER_NAMESPACE_END
