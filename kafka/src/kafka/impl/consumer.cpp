#include <kafka/impl/consumer.hpp>

#include <string_view>

#include <userver/formats/json/value_builder.hpp>
#include <userver/testsuite/testpoint.hpp>
#include <userver/tracing/span.hpp>
#include <userver/utils/async.hpp>

#include <kafka/impl/configuration.hpp>
#include <kafka/impl/consumer_impl.hpp>
#include <kafka/impl/stats.hpp>

USERVER_NAMESPACE_BEGIN

namespace kafka::impl {

Consumer::Consumer(std::unique_ptr<Configuration> configuration,
                   const std::vector<std::string>& topics,
                   std::size_t max_batch_size,
                   std::chrono::milliseconds poll_timeout,
                   bool enable_auto_commit,
                   engine::TaskProcessor& consumer_task_processor,
                   engine::TaskProcessor& main_task_processor)
    : component_name_(configuration->GetComponentName()),
      topics_(topics),
      max_batch_size_(max_batch_size),
      poll_timeout(poll_timeout),
      enable_auto_commit_(enable_auto_commit),
      consumer_task_processor_(consumer_task_processor),
      main_task_processor_(main_task_processor),
      consumer_(std::make_unique<ConsumerImpl>(std::move(configuration))) {}

Consumer::~Consumer() {
  static constexpr std::string_view kErrShutdownFailed{
      "Stop has somehow not been called, ConsumerScope leak?"};

  const bool shutdown_succeeded{!poll_task_.IsValid() ||
                                poll_task_.IsFinished()};

  if (!shutdown_succeeded) {
    UASSERT_MSG(false, kErrShutdownFailed);
    // in Release UASSERTs are skipped
    LOG_ERROR() << kErrShutdownFailed;
  }
}

ConsumerScope Consumer::MakeConsumerScope() { return ConsumerScope{*this}; }

void Consumer::DumpMetric(utils::statistics::Writer& writer) const {
  USERVER_NAMESPACE::kafka::impl::DumpMetric(writer, consumer_->GetStats());
}

void Consumer::StartMessageProcessing(ConsumerScope::Callback callback) {
  UINVARIANT(!processing_.exchange(true), "Message processing already started");

  poll_task_ = utils::Async(
      consumer_task_processor_, "consumer_polling",
      [this, callback = std::move(callback)] {
        ExtendCurrentSpan();

        consumer_->Subscribe(topics_);

        LOG_INFO() << fmt::format("Started messages polling");

        while (!engine::current_task::ShouldCancel()) {
          auto polled_messages = consumer_->PollBatch(
              max_batch_size_, engine::Deadline::FromDuration(poll_timeout));

          if (polled_messages.empty() || engine::current_task::ShouldCancel()) {
            /// @note Message batch may be not empty. It may be polled by
            /// another consumer in future, because they are not commited
            continue;
          }
          TESTPOINT(fmt::format("tp_{}_polled", component_name_), {});

          auto batch_processing_task =
              utils::Async(main_task_processor_, "messages_processing",
                           callback, utils::span{polled_messages});
          try {
            batch_processing_task.Get();

            consumer_->AccountMessageBatchProccessingSucceeded(polled_messages);
            TESTPOINT(fmt::format("tp_{}", component_name_), {});
          } catch (const std::exception& e) {
            consumer_->AccountMessageBatchProcessingFailed(polled_messages);

            const std::string error_text = e.what();
            LOG_ERROR() << fmt::format(
                "Messages processing failed in consumer: {}", error_text);
            TESTPOINT(fmt::format("tp_error_{}", component_name_),
                      [&error_text] {
                        formats::json::ValueBuilder error_json;
                        error_json["error"] = error_text;
                        return error_json.ExtractValue();
                      }());

            /// @note Messages must be destroyed, otherwise consumer won't stop
            /// and block forever
            polled_messages.clear();

            // Returning to last committed offsets
            LOG_INFO() << "Resubscribing after failure";
            consumer_->Resubscribe(topics_);
            processing_.store(true);
          }
        }
      });
}

void Consumer::AsyncCommit() {
  UINVARIANT(processing_.load(), "Message processing is not currently started");

  utils::Async(consumer_task_processor_, "consumer_committing", [this] {
    ExtendCurrentSpan();

    if (enable_auto_commit_) {
      LOG_WARNING() << "Manually commit invoked while `enable_auto_commit` "
                       "enabled. May cause an unexpected behaviour!!!";
    }

    /// @note Only schedules the offsets committment. Actual commit
    /// occurs in future, after some polling cycles.
    consumer_->AsyncCommit();
  }).Get();
}

void Consumer::Stop() noexcept {
  if (processing_.load() && poll_task_.IsValid()) {
    LOG_INFO() << "Consumer stopping";
    poll_task_.SyncCancel();
    LOG_INFO() << "Leaving group. Messages polling stopped";

    utils::Async(consumer_task_processor_, "consumer_stopping", [this] {
      ExtendCurrentSpan();

      consumer_->LeaveGroup();
      TESTPOINT(fmt::format("tp_{}_stopped", component_name_), {});
    }).Get();
    processing_.store(false);
    LOG_INFO() << "Consumer stopped";
  }
}

void Consumer::ExtendCurrentSpan() const {
  tracing::Span::CurrentSpan().AddTag("kafka_consumer", component_name_);
}

}  // namespace kafka::impl

USERVER_NAMESPACE_END
