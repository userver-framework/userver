#include <userver/kafka/consumer.hpp>

#include <userver/testsuite/testpoint.hpp>

#include <cppkafka/consumer.h>

#include <kafka/configuration.hpp>
#include <kafka/impl/stats.hpp>

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
      consumer_task_processor_(consumer_task_processor),
      main_task_processor_(main_task_processor),
      stats_(std::make_unique<impl::Stats>()),
      config_(SetErrorCallback(std::move(config), *stats_)),
      is_testsuite_mode_(is_testsuite_mode) {
  statistics_holder_ =
      storage.RegisterExtender(component_name, [this](const auto&) {
        return started_processing_.test()
                   ? ExtendStatistics(*stats_)
                   : formats::json::ValueBuilder{formats::json::Type::kObject}
                         .ExtractValue();
      });
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
      [this, callback = std::move(callback)] {
        Init();

        LOG_INFO() << fmt::format("Consumer '{}' started messages polling",
                                  component_name_);

        while (!engine::current_task::ShouldCancel()) {
          auto messages_polled = GetPolledMessages();
          if (messages_polled.empty() || engine::current_task::ShouldCancel()) {
            continue;
          }
          std::vector<std::shared_ptr<impl::TopicStats>> topics_stats;
          topics_stats.reserve(messages_polled.size());
          for (const auto& polled_message : messages_polled) {
            topics_stats.push_back(stats_->topics_stats[polled_message.topic]);
          }

          auto batch_processing_task =
              utils::Async(main_task_processor_, "messages_processing",
                           callback, std::move(messages_polled));
          try {
            batch_processing_task.Get();
            TESTPOINT(fmt::format("tp_{}", component_name_), {});

            for (const auto& topic_stats : topics_stats) {
              ++topic_stats->messages_counts.messages_success;
            }
          } catch (const std::exception& e) {
            for (const auto& topic_stats : topics_stats) {
              ++topic_stats->messages_counts.messages_error;
            }

            // Returning to last committed message
            // GetAssignment();
            consumer_.reset();
            LOG_INFO() << fmt::format(
                "Consumer '{}' living group. Messages polling stopped",
                component_name_);
            Init();
            started_processing_.test_and_set();

            const std::string error_text = e.what();
            LOG_ERROR() << fmt::format(
                "Messages processing failed in consumer '{}': {}",
                component_name_, error_text);
            TESTPOINT(fmt::format("tp_error_{}", component_name_),
                      [&error_text] {
                        formats::json::ValueBuilder error_json;
                        error_json["error"] = error_text;
                        return error_json.ExtractValue();
                      }());
          }
        }

        consumer_.reset();
        LOG_INFO() << fmt::format(
            "Consumer '{}' living group. Messages polling stopped",
            component_name_);
        started_processing_.clear();
        TESTPOINT(fmt::format("tp_{}_stopped", component_name_), {});
      });
}

void Consumer::AsyncCommit() {
  UASSERT_MSG(started_processing_.test(), "Message processing not yet started");

  utils::Async(consumer_task_processor_, "consumer_committing", [this] {
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
    if (messages_batch.empty()) {
      return {};
    }
    LOG_INFO() << "Polled batch of " << messages_batch.size() << " messages";
    TESTPOINT(fmt::format("tp_{}_polled", component_name_), {});
    messages_polled.reserve(messages_batch.size());

    auto& topics_stats = stats_->topics_stats;
    for (auto&& message : messages_batch) {
      MessagePolled message_polled{std::move(message)};

      const auto& topic = message_polled.topic;
      LOG_INFO() << fmt::format(
          "Message from kafka topic '{}' received by '{}': '{}' -> '{}' with "
          "partition {} by offset {}",
          topic, component_name_, message_polled.key, message_polled.payload,
          message_polled.partition, message_polled.offset);
      ++topics_stats[topic]->messages_counts.messages_total;

      const auto message_timestamp = message_polled.timestamp;
      if (message_timestamp) {
        const auto take_time =
            std::chrono::system_clock::now().time_since_epoch();
        const auto ms_duration =
            std::chrono::duration_cast<std::chrono::milliseconds>(
                take_time - message_timestamp.value())
                .count();
        topics_stats[topic]->avg_ms_spent_time.GetCurrentCounter().Account(
            ms_duration);
      } else {
        LOG_WARNING() << "No message timestamp in kafka message";
        continue;
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
  consumer_ = std::make_unique<cppkafka::Consumer>(*config_);
  LOG_INFO() << fmt::format("The consumer will be subscribed to {}",
                            fmt::join(topics_, ", "));

  consumer_->set_assignment_callback(
      [this](const cppkafka::TopicPartitionList& topic_partitions) {
        for (const auto& topic_partition : topic_partitions) {
          LOG_INFO() << fmt::format("The consumer '{}' was subscribed to ",
                                    component_name_)
                     << topic_partition.get_partition() << " partition of "
                     << topic_partition.get_topic() << " topic";
          TESTPOINT(fmt::format("tp_{}_subscribed", component_name_), {});
        }
      });
  consumer_->set_revocation_callback(
      [this](const cppkafka::TopicPartitionList& topic_partitions) {
        for (const auto& topic_partition : topic_partitions) {
          LOG_INFO() << fmt::format("The consumer '{}' was revoked ",
                                    component_name_)
                     << topic_partition.get_partition() << " partition of "
                     << topic_partition.get_topic() << " topic";
          TESTPOINT(fmt::format("tp_{}_revoked", component_name_), {});
        }
      });
  consumer_->set_rebalance_error_callback([this](const cppkafka::Error& error) {
    LOG_ERROR() << fmt::format("Error during consume '{}', rebalancing: {}",
                               component_name_, error.to_string());
  });

  consumer_->subscribe(topics_);
}

void Consumer::GetAssignment() {
  const auto assignment = consumer_->get_assignment();
  for (const auto& topic_partition : assignment) {
    LOG_INFO() << fmt::format("The consumer '{}' returning to ",
                              component_name_)
               << topic_partition.get_partition() << " partition of "
               << topic_partition.get_topic() << " topic by offset "
               << topic_partition.get_offset();
    TESTPOINT(fmt::format("tp_{}_assigned", component_name_), {});
  }
  consumer_->assign(assignment);
}

void Consumer::Stop() noexcept {
  if (poll_task_.IsValid()) {
    LOG_INFO() << fmt::format("Stopping '{}' consumer", component_name_);
    poll_task_.SyncCancel();
  }
}

}  // namespace kafka

USERVER_NAMESPACE_END
