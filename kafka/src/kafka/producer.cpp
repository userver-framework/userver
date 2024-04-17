#include <userver/kafka/producer.hpp>

#include <userver/formats/json/value_builder.hpp>
#include <userver/testsuite/testpoint.hpp>
#include <userver/utils/async.hpp>
#include <userver/utils/text_light.hpp>

#include <cppkafka/producer.h>

#include <kafka/configuration.hpp>
#include <kafka/impl/async_state.hpp>
#include <kafka/impl/common.hpp>

USERVER_NAMESPACE_BEGIN

namespace kafka {

Producer::Producer(std::unique_ptr<cppkafka::Configuration> config,
                   const std::string& component_name,
                   engine::TaskProcessor& producer_task_processor,
                   bool is_testsuite_mode, utils::statistics::Storage& storage)
    : config_(SetErrorCallback(std::move(config), stats_)),
      component_name_(component_name),
      producer_task_processor_(producer_task_processor),
      is_testsuite_mode_(is_testsuite_mode) {
  statistics_holder_ = storage.RegisterExtender(
      component_name, [this](const auto&) { return ExtendStatistics(stats_); });
}

void Producer::Init() {
  std::unique_lock<engine::Mutex> lock(mutex_);
  if (producer_ == nullptr) {
    producer_ = std::make_unique<cppkafka::Producer>(*config_);
    poll_task_ =
        utils::Async(producer_task_processor_, "producer_polling", [this] {
          for (;;) {
            producer_->poll(std::chrono::milliseconds(100));
            if (engine::current_task::IsCancelRequested()) {
              return;
            }
          }
        });
  }
}

void Producer::Send(std::string topic_name, std::string key,
                    std::string message,
                    const std::optional<int> partition_value) {
  SendInternal(std::move(topic_name), std::move(key), std::move(message),
               std::move(partition_value))
      .Get();
}

void Producer::SendAsync(std::string topic_name, std::string key,
                         std::string message,
                         const std::optional<int> partition_value) {
  SendInternal(std::move(topic_name), std::move(key), std::move(message),
               std::move(partition_value))
      .Detach();
}

Producer::~Producer() {
  LOG_INFO() << "Producer poll task is requested to cancel";
  if (poll_task_.IsValid()) {
    poll_task_.RequestCancel();
    poll_task_.Wait();
  }
}

[[nodiscard]] engine::TaskWithResult<void> Producer::SendInternal(
    std::string topic_name, std::string key, std::string message,
    const std::optional<int> partition_value) {
  LOG_INFO() << "Message to topic " << topic_name << " is requested to send";
  if (first_send_) {
    Init();
    first_send_ = false;
  }

  if (!poll_task_.IsValid() || poll_task_.IsFinished()) {
    throw std::runtime_error(
        "Polling task is invalid or finish, message is not sent");
  }

  return utils::Async(
      producer_task_processor_, "producer_producing",
      [this, topic_name = std::move(topic_name), partition_value,
       key = std::move(key), message = std::move(message)] {
        if (is_testsuite_mode_) {
          // Testpoint server does not accept non-utf8 data
          TESTPOINT(fmt::format("tp_{}", component_name_), [&] {
            formats::json::ValueBuilder builder;
            builder["key"] = key;
            if (utils::text::IsUtf8(message)) {
              builder["message"] = message;
            }
            return builder.ExtractValue();
          }());
          return;
        }

        auto state = std::make_unique<impl::AsyncState>(topic_name);
        auto future = state->promise.get_future();
        state->builder.payload(message);
        if (partition_value.has_value()) {
          state->builder.partition(partition_value.value());
        }
        state->builder.key(key);
        auto& messages_counts =
            stats_.topics_stats[topic_name]->messages_counts;
        ++messages_counts.messages_total;
        const auto start_time =
            std::chrono::system_clock::now().time_since_epoch();

        producer_->produce(state->builder);
        // 'produce' did not throw, therefore it now owns 'state'.
        [[maybe_unused]] const auto state_ptr = state.release();

        const cppkafka::Error error = future.get();
        auto finish_time = std::chrono::system_clock::now().time_since_epoch();
        auto ms_duration =
            std::chrono::duration_cast<std::chrono::milliseconds>(finish_time -
                                                                  start_time)
                .count();
        stats_.topics_stats[topic_name]
            ->avg_ms_spent_time.GetCurrentCounter()
            .Account(ms_duration);
        if (static_cast<bool>(error)) {
          ++messages_counts.messages_error;
          LOG_ERROR() << "Message with error was sent " << error.to_string();
          throw std::runtime_error(error.to_string());
        }
        LOG_INFO() << "Message to topic " << topic_name
                   << " was successfully sent";
      });
}

}  // namespace kafka

USERVER_NAMESPACE_END
