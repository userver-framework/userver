#include "topic_reader.hpp"

#include <fmt/format.h>

#include <userver/components/component.hpp>
#include <userver/engine/sleep.hpp>
#include <userver/logging/log.hpp>
#include <userver/testsuite/testpoint.hpp>
#include <userver/utils/async.hpp>
#include <userver/utils/overloaded.hpp>
#include <userver/utils/underlying_value.hpp>
#include <userver/yaml_config/merge_schemas.hpp>
#include <userver/ydb/component.hpp>
#include <userver/ydb/exceptions.hpp>
#include <userver/ydb/impl/cast.hpp>
#include <userver/ydb/topic.hpp>

namespace sample {

namespace {

NYdb::NTopic::TReadSessionSettings ConstructReadSessionSettings(
    const std::string& consumer_name, const std::vector<std::string>& topics) {
  NYdb::NTopic::TReadSessionSettings read_session_settings;
  read_session_settings.ConsumerName(ydb::impl::ToString(consumer_name));
  for (const auto& topic_path : topics) {
    read_session_settings.AppendTopics(ydb::impl::ToString(topic_path));
  }
  return read_session_settings;
}

class SessionReadTask {
 public:
  using TReadSessionEvent = NYdb::NTopic::TReadSessionEvent;
  using TSessionClosedEvent = NYdb::NTopic::TSessionClosedEvent;

  explicit SessionReadTask(ydb::TopicReadSession&& read_session)
      : read_session_(std::move(read_session)) {}

  ~SessionReadTask() {
    if (!session_closed_) {
      read_session_.Close(std::chrono::milliseconds{3000});
    }
  }

  void Run() {
    while (!engine::current_task::ShouldCancel() && !session_closed_) {
      try {
        LOG_INFO() << "Waiting events...";
        auto events = read_session_.GetEvents();

        LOG_INFO() << "Handling events...";
        for (auto& event : events) {
          HandleEvent(event);
        }
      } catch (const ydb::OperationCancelledError& /*ex*/) {
        break;
      }
    }
  }

 private:
  void HandleEvent(TReadSessionEvent::TEvent& event) {
    std::visit(
        utils::Overloaded{
            [this](TReadSessionEvent::TDataReceivedEvent& e) {
              NYdb::NTopic::TDeferredCommit deferredCommit;
              deferredCommit.Add(e);
              HandleDataReceivedEvent(e);
              // commit if HandleDataReceivedEvent succeeded
              deferredCommit.Commit();
            },
            [](TReadSessionEvent::TCommitOffsetAcknowledgementEvent&) {
              //
            },
            [](TReadSessionEvent::TStartPartitionSessionEvent& e) {
              LOG_DEBUG() << "Starting partition session [TopicPath="
                          << e.GetPartitionSession()->GetTopicPath()
                          << ", PartitionId="
                          << e.GetPartitionSession()->GetPartitionId() << "]";
              e.Confirm();  // partition assigned
            },
            [](TReadSessionEvent::TStopPartitionSessionEvent& e) {
              LOG_DEBUG() << "Stopping partition session [TopicPath="
                          << e.GetPartitionSession()->GetTopicPath()
                          << ", PartitionId="
                          << e.GetPartitionSession()->GetPartitionId() << "]";
              e.Confirm();  // partition revoked
            },
            [](TReadSessionEvent::TPartitionSessionClosedEvent& e) {
              if (TReadSessionEvent::TPartitionSessionClosedEvent::EReason::
                      StopConfirmedByUser != e.GetReason()) {
                LOG_WARNING()
                    << "Unexpected PartitionSessionClosedEvent [Reason="
                    << utils::UnderlyingValue(e.GetReason()) << "]";
                // partition lost
              }
            },
            [](TReadSessionEvent::TPartitionSessionStatusEvent&) {
              //
            },
            [this](NYdb::NTopic::TSessionClosedEvent& e) {
              LOG_INFO() << "Session closed";
              session_closed_ = true;
              if (!e.IsSuccess()) {
                throw std::runtime_error{"Session closed unsuccessfully: " +
                                         e.GetIssues().ToString()};
              }
            },
        },
        event);
  }

  void HandleDataReceivedEvent(TReadSessionEvent::TDataReceivedEvent& event) {
    LOG_DEBUG() << "Handle DataReceivedEvent [MessagesCount="
                << event.GetMessagesCount() << "]";
    for (const auto& message : event.GetMessages()) {
      HandleMessage(message);
    }
  }

  void HandleMessage(
      const TReadSessionEvent::TDataReceivedEvent::TMessage& message) {
    LOG_DEBUG() << "Handle Message [Offset=" << message.GetOffset()
                << ", Data: " << message.GetData() << "]";

    TESTPOINT("topic-handle-message",
              formats::json::FromString(message.GetData()));

    /// handle message...

    /*
      If it throws, read session will be closed and recreated in
      {restart_session_delay} ms All uncommited messages will be received again
    */
  }

 private:
  ydb::TopicReadSession read_session_;
  bool session_closed_{false};
};

class TopicReader {
 public:
  TopicReader(std::shared_ptr<ydb::TopicClient> topic_client,
              const NYdb::NTopic::TReadSessionSettings& read_session_settings,
              std::chrono::milliseconds restart_session_delay)
      : topic_client_{std::move(topic_client)},
        read_session_settings_{read_session_settings},
        restart_session_delay_{restart_session_delay} {}

  void Run() {
    while (!engine::current_task::ShouldCancel()) {
      try {
        LOG_INFO() << "Creating read session...";
        auto read_session =
            topic_client_->CreateReadSession(read_session_settings_);

        LOG_INFO() << "Starting session read...";
        SessionReadTask session_read_task{std::move(read_session)};
        session_read_task.Run();

        LOG_INFO() << "Session read finished";
      } catch (const std::exception& ex) {
        LOG_ERROR() << "Session read failed: " << ex;
      }

      engine::InterruptibleSleepFor(restart_session_delay_);
    }
  }

 private:
  const std::shared_ptr<ydb::TopicClient> topic_client_;
  const NYdb::NTopic::TReadSessionSettings read_session_settings_;
  const std::chrono::milliseconds restart_session_delay_;
};

}  // namespace

TopicReaderComponent::TopicReaderComponent(
    const components::ComponentConfig& config,
    const components::ComponentContext& context)
    : components::LoggableComponentBase(config, context) {
  const auto consumer_name = config["consumer-name"].As<std::string>();
  const auto topics = config["topics"].As<std::vector<std::string>>();
  const auto restart_session_delay =
      config["restart-session-delay"].As<std::chrono::milliseconds>();

  auto topic_reader = std::make_unique<TopicReader>(
      context.FindComponent<ydb::YdbComponent>().GetTopicClient("sampledb"),
      ConstructReadSessionSettings(consumer_name, topics),
      restart_session_delay);

  read_task_ = utils::CriticalAsync(
      config.Name() + "-read-task",
      [topic_reader = std::move(topic_reader)] { topic_reader->Run(); });
}

yaml_config::Schema TopicReaderComponent::GetStaticConfigSchema() {
  return yaml_config::MergeSchemas<components::LoggableComponentBase>(R"(
type: object
description: sample topic reader
additionalProperties: false
properties:
    consumer-name:
        type: string
        description: consumer name
    topics:
        type: array
        description: topics
        items:
            type: string
            description: topic path
    restart-session-delay:
        type: string
        description: restart session delay
  )");
}

}  // namespace sample
