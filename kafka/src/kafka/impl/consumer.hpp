#pragma once

#include <chrono>

#include <userver/engine/task/task.hpp>
#include <userver/engine/task/task_processor_fwd.hpp>
#include <userver/kafka/consumer_scope.hpp>
#include <userver/utils/statistics/writer.hpp>

USERVER_NAMESPACE_BEGIN

namespace kafka::impl {

class Configuration;
class ConsumerImpl;

class Consumer final {
 public:
  static constexpr std::chrono::milliseconds kDefaultPollTimeout{500};

  /// @brief Creates the Kafka Consumer.
  ///
  /// @note No messages processing starts. Use `ConsumerScope::Start` to launch
  /// the messages processing loop
  ///
  /// @param topics stands for topics list that consumer subscribes to
  /// after `ConsumerScope::Start` called
  /// @param max_batch_size stands for max polled message batches size
  /// @param poll_timeout is a timeout for one message batch polling loop
  /// @param enable_auto_commit enables automatic periodically offsets
  /// committing. Note: if enabled, `AsyncCommit` should not be called manually
  /// @param consumer_task_processor -- task processor for message batches
  /// polling
  /// All callbacks are invoked in `main_task_processor`
  Consumer(std::unique_ptr<Configuration> configuration,
           const std::vector<std::string>& topics, std::size_t max_batch_size,
           std::chrono::milliseconds poll_timeout, bool enable_auto_commit,
           engine::TaskProcessor& consumer_task_processor,
           engine::TaskProcessor& main_task_processor);

  /// @brief Cancels the consumer polling task and stop the underlying consumer.
  /// @see `ConsumerScope::Stop` for stopping process understanding
  ~Consumer();

  Consumer(Consumer&&) noexcept = delete;
  Consumer& operator=(Consumer&&) noexcept = delete;

  /// @brief Creates the RAII object for user consumer interaction.
  ConsumerScope MakeConsumerScope();

  /// @brief Dumps per topic messages processing statistics.
  /// @see impl/stats.hpp
  void DumpMetric(utils::statistics::Writer& writer) const;

 private:
  friend class kafka::ConsumerScope;

  /// @brief Subscribes for `topics_` and starts the `poll_task_`, in which
  /// periodically polls the message batches.
  void StartMessageProcessing(ConsumerScope::Callback callback);

  /// @brief Calls `poll_task_.SyncCancel()`
  void Stop() noexcept;

  /// @brief Schedules the committment task.
  /// @see `ConsumerScope::AsyncCommit` for better committment process
  /// understanding
  void AsyncCommit();

  /// @brief Adds consumer name to current span.
  void ExtendCurrentSpan() const;

 private:
  std::atomic<bool> processing_{false};

  const std::string component_name_;

  const std::vector<std::string> topics_;
  const std::size_t max_batch_size_{};
  const std::chrono::milliseconds poll_timeout{};
  const bool enable_auto_commit_{};

  engine::TaskProcessor& consumer_task_processor_;
  engine::TaskProcessor& main_task_processor_;

  std::unique_ptr<ConsumerImpl> consumer_;
  engine::Task poll_task_;
};

USERVER_NAMESPACE_END

}  // namespace kafka::impl
