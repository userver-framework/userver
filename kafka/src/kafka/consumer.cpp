#include <userver/kafka/consumer.hpp>

#include <userver/testsuite/testpoint.hpp>

#include <cppkafka/consumer.h>

#include <kafka/configuration.hpp>
#include <kafka/impl/common.hpp>

USERVER_NAMESPACE_BEGIN

namespace kafka {

MessagePolled::MessagePolled(cppkafka::Message&& message)
    : key(message.get_key()),
      payload(message.get_payload()),
      topic(message.get_topic()),
      partition(message.get_partition()),
      offset(message.get_offset()) {
  if (message.get_timestamp().has_value()) {
    timestamp = message.get_timestamp().value().get_timestamp();
  }
}

ConsumerScope::ConsumerScope(Consumer& consumer) noexcept
    : consumer_(consumer) {}

ConsumerScope::~ConsumerScope() { Stop(); }

void ConsumerScope::Start(Callback callback) {
  consumer_.StartMessageProcessing(std::move(callback));
}

void ConsumerScope::Stop() noexcept { consumer_.Stop(); }

void ConsumerScope::AsyncCommit() { consumer_.AsyncCommit(); }

Consumer::Consumer(std::unique_ptr<cppkafka::Configuration> config,
                   const std::string& component_name,
                   const std::vector<std::string>& topics,
                   const std::size_t max_batch_size,
                   engine::TaskProcessor& consumer_task_processor,
                   engine::TaskProcessor& main_task_processor,
                   bool is_testsuite_mode, utils::statistics::Storage& storage)
    : component_name_(component_name),
      topics_(topics),
      max_batch_size_(max_batch_size),
      consumer_(std::make_unique<cppkafka::Consumer>(
          *SetErrorCallback(std::move(config), stats_))),
      consumer_task_processor_(consumer_task_processor),
      main_task_processor_(main_task_processor),
      is_testsuite_mode_(is_testsuite_mode) {
  Init();

  statistics_holder_ = storage.RegisterExtender(
      component_name, [this](const auto&) { return ExtendStatistics(stats_); });
}

Consumer::~Consumer() {
  UASSERT_MSG(!poll_task_.IsValid() || poll_task_.IsFinished(),
              "Stop has somehow not been called, ConsumerScope leak?");
}

ConsumerScope Consumer::MakeConsumerScope() { return ConsumerScope{*this}; }

void Consumer::StartMessageProcessing(ConsumerScope::Callback callback) {
  UASSERT_MSG(!started_processing_.test_and_set(),
              "Message processing already started");

  poll_task_ = utils::Async(
      consumer_task_processor_, "consumer_polling",
      [this, callback = std::move(callback)]() mutable {
        LOG_INFO() << "Consumer started messages polling";
        for (;;) {
          if (engine::current_task::IsCancelRequested()) {
            return;
          }
          auto messages_polled = GetPolledMessages();
          if (messages_polled.empty()) {
            continue;
          }
          auto message_processing_task =
              utils::Async(main_task_processor_, "messages_processing",
                           std::move(callback), std::move(messages_polled));
          try {
            message_processing_task.Get();
            TESTPOINT(fmt::format("tp_{}", component_name_), {});
          } catch (const std::exception& e) {
            ++stats_.topics_stats[""]->messages_counts.messages_error;

            // Returning to last committed message
            GetAssigment();

            const std::string& error_text = e.what();
            LOG_ERROR() << "Message's processing failed: " << error_text;
            TESTPOINT(fmt::format("tp_error_{}", component_name_),
                      [&error_text]() {
                        formats::json::ValueBuilder error_json;
                        error_json["error"] = std::string(error_text);
                        return error_json.ExtractValue();
                      }());
          }
        }
      });

  LOG_INFO() << "Consumer started messages processing";
}

void Consumer::AsyncCommit() {
  utils::Async(consumer_task_processor_, "consumer_committing", [this]() {
    consumer_->async_commit();
  }).Get();
}

void Consumer::PushTestMessage(MessagePolled&& message) {
  tests_messages_.push(std::move(message));
}

std::vector<MessagePolled> Consumer::GetPolledMessages() {
  std::vector<MessagePolled> messages_polled;
  if (!is_testsuite_mode_) {
    std::vector<cppkafka::Message> messages_batch =
        consumer_->poll_batch(max_batch_size_, std::chrono::milliseconds(500));
    if (!messages_batch.empty()) {
      LOG_INFO() << "Polled batch of " << messages_batch.size() << " messages";
    }
    messages_polled.reserve(messages_batch.size());
    for (auto&& message : messages_batch) {
      MessagePolled message_polled{std::move(message)};

      const auto& topic = message_polled.topic;
      LOG_INFO() << "Message from kafka topic " << topic << " received";
      ++stats_.topics_stats[topic]->messages_counts.messages_total;

      const auto message_timestamp = message_polled.timestamp;
      if (message_timestamp) {
        auto take_time = std::chrono::system_clock::now().time_since_epoch();
        auto ms_duration =
            std::chrono::duration_cast<std::chrono::milliseconds>(
                take_time - message_timestamp.value())
                .count();
        stats_.topics_stats[topic]
            ->avg_ms_spent_time.GetCurrentCounter()
            .Account(ms_duration);
      } else {
        LOG_WARNING() << "No message timestamp in kafka message";
      }
      messages_polled.push_back(std::move(message_polled));
    }
    return messages_polled;
  }
  for (std::size_t i = 0; i < max_batch_size_; ++i) {
    if (tests_messages_.empty()) {
      break;
    }
    messages_polled.push_back(std::move(tests_messages_.front()));
    tests_messages_.pop();
  }
  return messages_polled;
}

void Consumer::Init() {
  consumer_->set_assignment_callback(
      [](const cppkafka::TopicPartitionList& topic_partitions) {
        for (const auto& topic_partition : topic_partitions) {
          LOG_INFO() << "The consumer was subscribed to "
                     << topic_partition.get_partition() << " partition of "
                     << topic_partition.get_topic() << " topic";
        }
      });
  consumer_->set_revocation_callback(
      [](const cppkafka::TopicPartitionList& topic_partitions) {
        for (const auto& topic_partition : topic_partitions) {
          LOG_INFO() << "The consumer was revoked "
                     << topic_partition.get_partition() << " partition of "
                     << topic_partition.get_topic() << " topic";
        }
      });
  consumer_->set_rebalance_error_callback([](const cppkafka::Error& error) {
    LOG_ERROR() << fmt::format("Error during rebalancing: {}",
                               error.to_string());
  });

  LOG_INFO() << fmt::format("The consumer will be subscribed to {}",
                            fmt::join(topics_, ", "));

  consumer_->subscribe(topics_);
}

void Consumer::GetAssigment() {
  const auto assignment = consumer_->get_assignment();
  consumer_->assign(assignment);
}

void Consumer::Stop() noexcept { poll_task_.SyncCancel(); }

}  // namespace kafka

USERVER_NAMESPACE_END
