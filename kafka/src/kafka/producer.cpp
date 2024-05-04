#include <userver/kafka/producer.hpp>

#include <userver/formats/json/value_builder.hpp>
#include <userver/testsuite/testpoint.hpp>
#include <userver/utils/async.hpp>
#include <userver/utils/text_light.hpp>

#include <kafka/impl/configuration.hpp>
#include <kafka/impl/producer_impl.hpp>

USERVER_NAMESPACE_BEGIN

namespace kafka {

Producer::Producer(std::unique_ptr<Configuration> configuration,
                   const std::string& component_name,
                   engine::TaskProcessor& producer_task_processor,
                   std::chrono::milliseconds poll_timeout,
                   std::size_t send_retries,
                   utils::statistics::Storage& storage)
    : component_name_(component_name),
      producer_task_processor_(producer_task_processor),
      log_tags_({{"kafka_producer", component_name}}),
      configuration_(std::move(configuration)),
      poll_timeout_(poll_timeout),
      send_retries_(send_retries),
      storage_(storage) {
  statistics_holder_ =
      storage_.RegisterExtender(component_name_, [this](const auto&) {
        return !first_send_.load()
                   ? ExtendStatistics(producer_->GetStats())
                   : formats::json::ValueBuilder{formats::json::Type::kObject}
                         .ExtractValue();
      });
}

Producer::~Producer() {
  LOG_INFO() << log_tags_ << "Producer poll task is requested to cancel";
  if (poll_task_.IsValid()) {
    poll_task_.RequestCancel();
    poll_task_.Wait();
  }

  statistics_holder_.Unregister();

  utils::Async(producer_task_processor_, "producer_shutdown", [this] {
    producer_.reset();
  }).Get();
}

void Producer::InitProducerAndStartPollingIfFirstSend() const {
  if (first_send_.load()) {
    static engine::Mutex init_lock;
    std::lock_guard lock{init_lock};

    if (producer_ == nullptr) {
      producer_ = configuration_->MakeProducerImpl(log_tags_);

      poll_task_ =
          utils::Async(producer_task_processor_, "producer_polling", [this] {
            while (!engine::current_task::ShouldCancel()) {
              producer_->Poll(poll_timeout_);
            }
          });

      LOG_INFO() << log_tags_ << "Producer initialized";
      first_send_.store(false);
    }
  }

  UASSERT_MSG(!first_send_.load() && producer_,
              "Unsychronized producer configuration");

  VerifyNotFinished();
}

void Producer::Send(const std::string& topic_name, std::string_view key,
                    std::string_view message,
                    std::optional<std::uint32_t> partition) const {
  InitProducerAndStartPollingIfFirstSend();

  LOG_INFO() << log_tags_
             << fmt::format("Message to topic '{}' is requested to send",
                            topic_name);

  utils::Async(producer_task_processor_, "producer_send",
               [this, &topic_name, key, message, partition] {
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

  LOG_INFO() << log_tags_
             << fmt::format("Message to topic '{}' is requested to send",
                            topic_name);

  return utils::Async(
      producer_task_processor_, "producer_send_async",
      [this, topic_name = std::move(topic_name), key = std::move(key),
       message = std::move(message), partition] {
        producer_->Send(topic_name, key, message, partition, send_retries_);

        SendToTestPoint(topic_name, key, message);
      });
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

}  // namespace kafka

USERVER_NAMESPACE_END
