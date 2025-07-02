#pragma once

#include <chrono>
#include <memory>

#include <userver/engine/task/task.hpp>
#include <userver/engine/task/task_processor_fwd.hpp>
#include <userver/kafka/consumer_scope.hpp>
#include <userver/kafka/impl/consumer_params.hpp>
#include <userver/kafka/impl/holders.hpp>
#include <userver/kafka/impl/stats.hpp>
#include <userver/utils/statistics/writer.hpp>

USERVER_NAMESPACE_BEGIN

namespace kafka::impl {

class ConsumerImpl;

struct ConsumerConfiguration;
struct Secret;

class Consumer final {
public:
    /// @brief Creates the Kafka Consumer.
    ///
    /// @note No messages processing starts. Use ConsumerScope::Start to launch
    /// the messages processing loop
    ///
    /// @param topics stands for topics list that consumer subscribes to
    /// after ConsumerScope::Start called
    /// @param consumer_task_processor -- task processor for message batches polling
    /// @param consumer_blocking_task_processor -- task processor for consumer blocking operations
    /// All callbacks are invoked in `main_task_processor`
    Consumer(
        const std::string& name,
        const std::vector<std::string>& topics,
        engine::TaskProcessor& consumer_task_processor,
        engine::TaskProcessor& consumer_blocking_task_processor,
        engine::TaskProcessor& main_task_processor,
        const ConsumerConfiguration& consumer_configuration,
        const Secret& secrets,
        ConsumerExecutionParams params
    );

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

    /// @cond
    /// @brief Retrieves the low and high offsets for the specified topic and partition.
    /// @see ConsumerScope::GetOffsetRange for better commitment process
    OffsetRange GetOffsetRange(
        const std::string& topic,
        std::uint32_t partition,
        std::optional<std::chrono::milliseconds> timeout = std::nullopt
    ) const;

    /// @brief Retrieves the partition IDs for the specified topic.
    /// @see ConsumerScope::GetPartitionIds for better commitment process
    std::vector<std::uint32_t>
    GetPartitionIds(const std::string& topic, std::optional<std::chrono::milliseconds> timeout = std::nullopt) const;
    /// @endcond

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

    /// @brief Subscribes for configured topics and starts polling loop.
    void RunConsuming(ConsumerScope::Callback callback);

private:
    std::atomic<bool> processing_{false};
    Stats stats_;

    const std::string name_;
    const std::vector<std::string> topics_;
    const ConsumerExecutionParams execution_params;

    engine::TaskProcessor& consumer_task_processor_;
    engine::TaskProcessor& consumer_blocking_task_processor_;
    engine::TaskProcessor& main_task_processor_;

    ConfHolder conf_;
    std::unique_ptr<ConsumerImpl> consumer_;

    engine::Task poll_task_;
};

}  // namespace kafka::impl

USERVER_NAMESPACE_END
