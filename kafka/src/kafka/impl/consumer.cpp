#include <userver/kafka/impl/consumer.hpp>

#include <string_view>

#include <userver/engine/sleep.hpp>
#include <userver/formats/json/value_builder.hpp>
#include <userver/kafka/impl/configuration.hpp>
#include <userver/kafka/impl/stats.hpp>
#include <userver/testsuite/testpoint.hpp>
#include <userver/tracing/span.hpp>
#include <userver/utils/async.hpp>
#include <userver/utils/scope_guard.hpp>
#include <userver/yaml_config/yaml_config.hpp>

#include <kafka/impl/consumer_impl.hpp>

USERVER_NAMESPACE_BEGIN

namespace kafka::impl {

namespace {

void CallErrorTestpoint(const std::string& testpoint_name, const std::string& error_text) {
    TESTPOINT(testpoint_name, [&error_text] {
        formats::json::ValueBuilder error_json;
        error_json["error"] = error_text;
        return error_json.ExtractValue();
    }());
}

std::function<void()> CreateDurationNotifier(std::chrono::milliseconds max_callback_duration) {
    return [max_callback_duration, start_time = std::chrono::system_clock::now()]() {
        const auto callback_duration =
            std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::system_clock::now() - start_time);

        if (callback_duration > max_callback_duration / 2) {
            LOG_WARNING(
                "Your callback duration is {}ms. If callback duration "
                "exceeds the {}ms your consumer will be kicked from the "
                "group. If it is okey to have such a long callbacks, "
                "increase the `max_callback_duration` configuration option.",
                callback_duration.count(),
                max_callback_duration.count()
            );
        }
    };
}

}  // namespace

Consumer::Consumer(
    const std::string& name,
    const std::vector<std::string>& topics,
    engine::TaskProcessor& consumer_task_processor,
    engine::TaskProcessor& consumer_blocking_task_processor,
    engine::TaskProcessor& main_task_processor,
    const ConsumerConfiguration& configuration,
    const Secret& secrets,
    ConsumerExecutionParams params
)
    : name_(name),
      topics_(topics),
      execution_params_(params),
      consumer_task_processor_(consumer_task_processor),
      consumer_blocking_task_processor_(consumer_blocking_task_processor),
      main_task_processor_(main_task_processor),
      conf_(Configuration{name, configuration, secrets}.Release()),
      consumer_(std::make_unique<ConsumerImpl>(name_, conf_, topics_, params, rebalance_callback_, stats_)) {
    /// To check configuration validity
    [[maybe_unused]] auto _ = ConsumerHolder{conf_};
}

Consumer::~Consumer() {
    static constexpr std::string_view kErrShutdownFailed{"Stop has somehow not been called, ConsumerScope leak?"};

    const bool shutdown_succeeded{!poll_task_.IsValid() || poll_task_.IsFinished()};

    if (!shutdown_succeeded) {
        UASSERT_MSG(false, kErrShutdownFailed);
        // in Release UASSERTs are skipped
        LOG_ERROR() << kErrShutdownFailed;
    }
}

ConsumerScope Consumer::MakeConsumerScope() { return ConsumerScope{*this}; }

void Consumer::DumpMetric(utils::statistics::Writer& writer) const {
    USERVER_NAMESPACE::kafka::impl::DumpMetric(writer, stats_, this->name_);
}

void Consumer::RunConsuming(ConsumerScope::Callback callback) {
    // note: Consumer must be recreated after each stop,
    // because stop invalidates some internal consumer state (in librdkafka).
    // Nevertheless, it is possible to use blocking consumer methods after stop.
    consumer_ = std::make_unique<ConsumerImpl>(name_, conf_, topics_, execution_params_, rebalance_callback_, stats_);
    consumer_->StartConsuming();

    LOG_INFO("Started messages polling");

    while (!engine::current_task::ShouldCancel()) {
        auto polled_messages = consumer_->PollBatch(
            execution_params_.max_batch_size, engine::Deadline::FromDuration(execution_params_.poll_timeout)
        );

        if (engine::current_task::ShouldCancel()) {
            // Message batch may be not empty. The messages will be polled by
            // another consumer in future, because they are not committed
            LOG_DEBUG("Stopping consuming because of cancel");
            break;
        }
        if (polled_messages.empty()) {
            continue;
        }
        const engine::TaskCancellationBlocker cancelation_blocker;

        TESTPOINT(fmt::format("tp_{}_polled", name_), {});

        auto batch_processing_task =
            utils::Async(main_task_processor_, "messages_processing", callback, utils::span{polled_messages});
        const utils::ScopeGuard callback_duration_notifier{
            CreateDurationNotifier(execution_params_.max_callback_duration)};

        try {
            batch_processing_task.Get();

            consumer_->AccountMessageBatchProcessingSucceeded(polled_messages);
            TESTPOINT(fmt::format("tp_{}", name_), {});
        } catch (const std::exception& e) {
            consumer_->AccountMessageBatchProcessingFailed(polled_messages);
            throw;
        }
    }
}

void Consumer::StartMessageProcessing(ConsumerScope::Callback callback) {
    UINVARIANT(!processing_.exchange(true), "Message processing already started");

    poll_task_ =
        utils::CriticalAsync(consumer_task_processor_, "consumer_polling", [this, callback = std::move(callback)] {
            ExtendCurrentSpan();

            while (!engine::current_task::ShouldCancel()) {
                try {
                    RunConsuming(callback);
                } catch (const std::exception& e) {
                    LOG_ERROR("Messages processing failed in consumer: {}", e.what());

                    CallErrorTestpoint(fmt::format("tp_error_{}", name_), e.what());
                }

                if (engine::current_task::ShouldCancel()) {
                    break;
                }

                LOG_WARNING("Restarting consumer after {}ms...", execution_params_.restart_after_failure_delay.count());
                engine::InterruptibleSleepFor(execution_params_.restart_after_failure_delay);
            }
        });
}

void Consumer::AsyncCommit() {
    UINVARIANT(processing_.load(), "Message processing is not currently started");

    utils::Async(consumer_task_processor_, "consumer_committing", [this] {
        ExtendCurrentSpan();

        /// @note Only schedules the offsets commitment. Actual commit
        /// occurs in future, after some polling cycles.
        consumer_->AsyncCommit();
    }).Get();
}

OffsetRange Consumer::GetOffsetRange(
    utils::zstring_view topic,
    std::uint32_t partition,
    std::optional<std::chrono::milliseconds> timeout
) const {
    return utils::Async(
               consumer_blocking_task_processor_,
               "consumer_getting_offset",
               [this, topic, partition, timeout] {
                   ExtendCurrentSpan();

                   return consumer_->GetOffsetRange(topic, partition, timeout);
               }
    ).Get();
}

std::vector<std::uint32_t>
Consumer::GetPartitionIds(utils::zstring_view topic, std::optional<std::chrono::milliseconds> timeout) const {
    return utils::Async(
               consumer_blocking_task_processor_,
               "consumer_getting_partition_ids",
               [this, topic, timeout] {
                   ExtendCurrentSpan();

                   return consumer_->GetPartitionIds(topic, timeout);
               }
    ).Get();
}

void Consumer::Seek(
    utils::zstring_view topic,
    std::uint32_t partition_id,
    std::uint64_t offset,
    std::chrono::milliseconds timeout
) const {
    UINVARIANT(processing_.load(), "Message processing is not currently started");

    return utils::Async(
               consumer_blocking_task_processor_,
               "consumer_seek",
               [this, topic, partition_id, offset, timeout] {
                   ExtendCurrentSpan();

                   return consumer_->Seek(topic, partition_id, offset, timeout);
               }
    ).Get();
}

void Consumer::SeekToBeginning(utils::zstring_view topic, std::uint32_t partition_id, std::chrono::milliseconds timeout)
    const {
    UINVARIANT(processing_.load(), "Message processing is not currently started");

    return utils::Async(
               consumer_blocking_task_processor_,
               "consumer_seek_to_beginning",
               [this, topic, partition_id, timeout] {
                   ExtendCurrentSpan();

                   return consumer_->SeekToBeginning(topic, partition_id, timeout);
               }
    ).Get();
}

void Consumer::SeekToEnd(utils::zstring_view topic, std::uint32_t partition_id, std::chrono::milliseconds timeout)
    const {
    UINVARIANT(processing_.load(), "Message processing is not currently started");

    return utils::Async(
               consumer_blocking_task_processor_,
               "consumer_seek_to_end",
               [this, topic, partition_id, timeout] {
                   ExtendCurrentSpan();

                   return consumer_->SeekToEnd(topic, partition_id, timeout);
               }
    ).Get();
}

void Consumer::SetRebalanceCallback(ConsumerRebalanceCallback rebalance_callback) {
    UINVARIANT(
        !processing_.load(),
        "Message processing is already started. Rebalance callback should be set before Start() call or after Stop() "
        "call ."
    );

    rebalance_callback_ = std::move(rebalance_callback);
}

void Consumer::ResetRebalanceCallback() {
    UINVARIANT(
        !processing_.load(),
        "Message processing is already started. Rebalance callback should be set before Start() call or after Stop() "
        "call ."
    );

    rebalance_callback_.reset();
}

void Consumer::Stop() noexcept {
    if (processing_.exchange(false) && poll_task_.IsValid()) {
        LOG_INFO("Stopping consumer poll task");
        poll_task_.SyncCancel();

        LOG_INFO("Stopping consumer");
        utils::Async(consumer_task_processor_, "consumer_stopping", [this] {
            // 1. This is blocking.
            // 2. This calls testpoints
            consumer_->StopConsuming();
        }).Get();
        TESTPOINT(fmt::format("tp_{}_stopped", name_), {});
        LOG_INFO("Consumer stopped");
    }
}

void Consumer::ExtendCurrentSpan() const { tracing::Span::CurrentSpan().AddTag("kafka_consumer", name_); }

}  // namespace kafka::impl

USERVER_NAMESPACE_END
