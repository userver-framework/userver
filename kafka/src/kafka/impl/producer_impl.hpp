#pragma once

#include <chrono>
#include <optional>

#include <kafka/impl/stats.hpp>

USERVER_NAMESPACE_BEGIN

namespace kafka {

using namespace std::chrono_literals;

namespace impl {

class ProducerHolder;

struct StatsAndLogging {
  const logging::LogExtra& log_tags_;
  mutable std::unique_ptr<impl::Stats> stats;
};

}  // namespace impl

class ProducerImpl {
  static constexpr auto kCoolDownFlushTimeout = 2s;

 public:
  /// @param conf is expected to point to `rd_kafka_t` object
  ProducerImpl(void* conf,
               std::unique_ptr<impl::StatsAndLogging> stats_and_logging_);

  ~ProducerImpl();

  const impl::Stats& GetStats() const;
  const logging::LogExtra& GetLoggingTags() const;

  /// @brief Send the message and waits for its delivery.
  void Send(const std::string& topic_name, std::string_view key,
            std::string_view message, std::optional<int> partition) const;

  /// @brief Polls for delivery events for @param poll_timeout_ milliseconds
  void Poll(std::chrono::milliseconds poll_timeout) const;

 private:
  std::unique_ptr<impl::ProducerHolder> producer_;
  std::unique_ptr<impl::StatsAndLogging> stats_and_logging_;
};

}  // namespace kafka

USERVER_NAMESPACE_END
