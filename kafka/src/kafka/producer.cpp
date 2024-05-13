#include <userver/kafka/producer.hpp>

#include <userver/formats/json/value_builder.hpp>
#include <userver/testsuite/testpoint.hpp>
#include <userver/tracing/span.hpp>
#include <userver/utils/async.hpp>
#include <userver/utils/text_light.hpp>

#include <kafka/impl/configuration.hpp>
#include <kafka/impl/producer_impl.hpp>
#include <kafka/impl/stats.hpp>

USERVER_NAMESPACE_BEGIN

namespace kafka {

Producer::Producer(std::unique_ptr<impl::Configuration> configuration,
                   engine::TaskProcessor& producer_task_processor,
                   std::chrono::milliseconds poll_timeout,
                   std::size_t send_retries)
    : component_name_(configuration->GetComponentName()),
      producer_task_processor_(producer_task_processor),
      poll_timeout_(poll_timeout),
      send_retries_(send_retries),
      configuration_(std::move(configuration)) {}

Producer::~Producer() {
  LOG_INFO() << "Producer poll task is requested to cancel";
  if (poll_task_.IsValid()) {
    poll_task_.SyncCancel();
  }

  utils::Async(producer_task_processor_, "producer_shutdown", [this] {
    producer_.reset();
  }).Get();
}

void Producer::InitProducerAndStartPollingIfFirstSend() const {
  if (first_send_.load()) {
    static engine::Mutex init_lock;
    std::lock_guard lock{init_lock};

    if (producer_ == nullptr) {
      producer_ =
          std::make_unique<impl::ProducerImpl>(std::move(configuration_));

      poll_task_ =
          utils::Async(producer_task_processor_, "producer_polling", [this] {
            ExtendCurrentSpan();

            LOG_INFO() << "Producer started polling";

            while (!engine::current_task::ShouldCancel()) {
              producer_->Poll(poll_timeout_);
            }
          });

      first_send_.store(false);
    }
  }

  UINVARIANT(!first_send_.load() && producer_,
             "Unsychronized producer configuration");

  VerifyNotFinished();
}

void Producer::Send(const std::string& topic_name, std::string_view key,
                    std::string_view message,
                    std::optional<std::uint32_t> partition) const {
  InitProducerAndStartPollingIfFirstSend();

  utils::Async(producer_task_processor_, "producer_send",
               [this, &topic_name, key, message, partition] {
                 ExtendCurrentSpan();

                 producer_->Send(topic_name, key, message, partition,
                                 send_retries_);

                 SendToTestPoint(topic_name, key, message);
               })
      .Get();
}

engine::TaskWithResult<void> Producer::SendAsync(
    std::string topic_name, std::string key, std::string message,
    std::optional<std::uint32_t> partition) const {
  InitProducerAndStartPollingIfFirstSend();

  return utils::Async(
      producer_task_processor_, "producer_send_async",
      [this, topic_name = std::move(topic_name), key = std::move(key),
       message = std::move(message), partition] {
        ExtendCurrentSpan();

        producer_->Send(topic_name, key, message, partition, send_retries_);

        SendToTestPoint(topic_name, key, message);
      });
}

void Producer::DumpMetric(utils::statistics::Writer& writer) const {
  if (!first_send_.load()) {
    impl::DumpMetric(writer, producer_->GetStats());
  }
}

void Producer::VerifyNotFinished() const {
  if (!poll_task_.IsValid() || poll_task_.IsFinished()) {
    throw std::runtime_error{
        "Polling task is invalid or finished, message is not sent"};
  }
}

void Producer::SendToTestPoint(std::string_view topic_name,
                               std::string_view key,
                               std::string_view message) const {
  // Testpoint server does not accept non-utf8 data
  TESTPOINT(fmt::format("tp_{}", component_name_), [&] {
    formats::json::ValueBuilder builder;
    builder["topic"] = topic_name;
    builder["key"] = key;
    if (utils::text::IsUtf8(message)) {
      builder["message"] = message;
    }
    return builder.ExtractValue();
  }());
}

void Producer::ExtendCurrentSpan() const {
  tracing::Span::CurrentSpan().AddTag("kafka_producer", component_name_);
}

}  // namespace kafka

USERVER_NAMESPACE_END
