#include "topic_writer_config.hpp"

#include <userver/ydb/component.hpp>

#include <userver/utils/assert.hpp>

USERVER_NAMESPACE_BEGIN

namespace ydb::impl {

namespace {
constexpr std::size_t kInternalMsgQueueSize{1000};
constexpr std::size_t kInternalYdbControlQueueSize{10};
constexpr std::size_t kWorkerNums{1};

struct TopicWriterComponentEntryConfig final {
    std::string topic_name;
    std::string database;
    std::size_t workers_num{kWorkerNums};
    std::size_t max_incoming_msg_queue_size{kInternalMsgQueueSize};
    std::size_t max_ydb_control_event_queue_size{kInternalYdbControlQueueSize};
};

TopicWriterComponentEntryConfig
Parse(const yaml_config::YamlConfig& config, formats::parse::To<TopicWriterComponentEntryConfig>) {
    TopicWriterComponentEntryConfig result;
    result.topic_name = config["topic"].As<std::string>();
    result.database = config["database"].As<std::string>();
    result.workers_num = config["internal-workers-num"].As<std::size_t>(result.workers_num);
    result.max_incoming_msg_queue_size =
        config["internal-msg-queue-size"].As<std::size_t>(result.max_incoming_msg_queue_size);
    result.max_ydb_control_event_queue_size =
        config["internal-ydb-control-queue-size"].As<std::size_t>(result.max_ydb_control_event_queue_size);
    return result;
}

TopicWriterSettings ParseTopicWriterSettings(
    const yaml_config::YamlConfig& config,
    const ydb::YdbComponent& ydb_component
) {
    const auto parsed_config = config.As<TopicWriterComponentEntryConfig>();
    auto topic_client = ydb_component.GetTopicClient(parsed_config.database);

    return TopicWriterSettings{
        .topic_client = std::move(topic_client),
        .topic_name = parsed_config.topic_name,
        .workers_num = parsed_config.workers_num,
        .max_incoming_msg_queue_size = parsed_config.max_incoming_msg_queue_size,
        .max_ydb_control_event_queue_size = parsed_config.max_ydb_control_event_queue_size,
    };
}
}  // namespace

std::unordered_map<std::string, ydb::TopicWriterSettings> MakeTopicWriterSettings(
    const components::ComponentConfig& config,
    const components::ComponentContext& context
) {
    std::unordered_map<std::string, ydb::TopicWriterSettings> settings;
    auto& ydb_component = context.FindComponent<ydb::YdbComponent>();

    const auto& topics_config = config["topics"];
    for (const auto& [name, individual_config] : USERVER_NAMESPACE::formats::common::Items(topics_config)) {
        settings.emplace(name, ParseTopicWriterSettings(individual_config, ydb_component));
    }

    return settings;
}

}  // namespace ydb::impl

USERVER_NAMESPACE_END
