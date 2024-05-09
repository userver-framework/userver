#pragma once

#include <chrono>
#include <cstdint>
#include <optional>

#include <kafka/impl/opaque.hpp>

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
            std::size_t retries) const;

  /// @brief Polls for delivery events for @param poll_timeout_ milliseconds
  void Poll(std::chrono::milliseconds poll_timeout) const;

  const Stats& GetStats() const;

 private:
  enum class SendResult {
    Succeeded,
    Failed,
    Retryable,
  };

  SendResult SendImpl(const std::string& topic_name, std::string_view key,
                      std::string_view message,
                      std::optional<std::uint32_t> partition) const;

 private:
  Opaque opaque_;

  class ProducerHolder;
  std::unique_ptr<ProducerHolder> producer_;
};

}  // namespace kafka::impl

USERVER_NAMESPACE_END
