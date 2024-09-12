#pragma once

#include <chrono>

#include <userver/engine/task/task.hpp>
#include <userver/engine/task/task_processor_fwd.hpp>
#include <userver/kafka/consumer_scope.hpp>
#include <userver/utils/statistics/writer.hpp>

USERVER_NAMESPACE_BEGIN

namespace kafka::impl {

class ConsumerImpl;

struct ConsumerConfiguration;
struct Secret;

/// @brief Parameters Consumer uses in runtime.
/// The struct is used only for documentation purposes, Consumer can be
/// created through ConsumerComponent.
struct ConsumerExecutionParams final {
  /// @brief stands for max polled message batches size
  std::size_t max_batch_size{1};

  /// @brief timeout for one message batch polling loop
  std::chrono::milliseconds poll_timeout{500};
};

class Consumer final {
 public:
  /// @brief Creates the Kafka Consumer.
  ///
  /// @note No messages processing starts. Use ConsumerScope::Start to launch
  /// the messages processing loop
  ///
  /// @param topics stands for topics list that consumer subscribes to
  /// after ConsumerScope::Start called
  /// @param consumer_task_processor -- task processor for message batches
  /// polling
  /// All callbacks are invoked in `main_task_processor`
  Consumer(const std::string& name, const std::vector<std::string>& topics,
           engine::TaskProcessor& consumer_task_processor,
           engine::TaskProcessor& main_task_processor,
           const ConsumerConfiguration& consumer_configuration,
           const Secret& secrets, ConsumerExecutionParams params);

  /// @brief Cancels the consumer polling task and stop the underlying consumer.
  /// @see ConsumerScope::Stop for stopping process understanding
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

  /// @brief Calls `poll_task_.SyncCancel()` and waits until consumer stopped.
  void Stop() noexcept;

  /// @brief Schedules the commitment task.
  /// @see ConsumerScope::AsyncCommit for better commitment process
  /// understanding
  void AsyncCommit();

  /// @brief Adds consumer name to current span.
  void ExtendCurrentSpan() const;

 private:
  std::atomic<bool> processing_{false};

  const std::string component_name_;

  const std::vector<std::string> topics_;
  const std::size_t max_batch_size_{};
  const std::chrono::milliseconds poll_timeout_{};

  engine::TaskProcessor& consumer_task_processor_;
  engine::TaskProcessor& main_task_processor_;

  std::unique_ptr<ConsumerImpl> consumer_;
  engine::Task poll_task_;
};

USERVER_NAMESPACE_END

}  // namespace kafka::impl
