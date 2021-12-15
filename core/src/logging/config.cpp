#include "config.hpp"

#include <stdexcept>

#include <userver/logging/level_serialization.hpp>
#include <userver/yaml_config/yaml_config.hpp>

USERVER_NAMESPACE_BEGIN

namespace logging {

LoggerConfig::QueueOveflowBehavior Parse(
    const yaml_config::YamlConfig& value,
    formats::parse::To<LoggerConfig::QueueOveflowBehavior>) {
  const auto overflow_behavior_name = value.As<std::string>();
  if (overflow_behavior_name == "discard")
    return LoggerConfig::QueueOveflowBehavior::kDiscard;
  if (overflow_behavior_name == "block")
    return LoggerConfig::QueueOveflowBehavior::kBlock;
  throw std::runtime_error("Unknown queue overflow behavior '" +
                           overflow_behavior_name +
                           "' (must be one of 'block', 'discard')");
}

LoggerConfig Parse(const yaml_config::YamlConfig& value,
                   formats::parse::To<LoggerConfig>) {
  LoggerConfig config;
  config.file_path = value["file_path"].As<std::string>();

  config.level = value["level"].As<logging::Level>(Level::kInfo);

  config.pattern =
      value["pattern"].As<std::string>(LoggerConfig::kDefaultPattern);

  config.flush_level = value["flush_level"].As<logging::Level>(Level::kWarning);

  config.message_queue_size = value["message_queue_size"].As<size_t>(
      LoggerConfig::kDefaultMessageQueueSize);
  if (config.message_queue_size & (config.message_queue_size - 1)) {
    throw std::runtime_error("log message queue size must be a power of 2");
  }

  config.queue_overflow_behavior =
      value["overflow_behavior"].As<LoggerConfig::QueueOveflowBehavior>(
          LoggerConfig::QueueOveflowBehavior::kDiscard);

  config.thread_pool_size = value["thread_pool_size"].As<size_t>(
      LoggerConfig::kDefaultThreadPoolSize);

  return config;
}

}  // namespace logging

USERVER_NAMESPACE_END
