#include "config.hpp"

#include <stdexcept>

#include <yaml_config/value.hpp>

namespace logging {
namespace {

LoggerConfig::QueueOveflowBehavior OverflowBehaviorFromString(
    const std::string& overflow_behavior_name) {
  if (overflow_behavior_name == "discard")
    return LoggerConfig::QueueOveflowBehavior::kDiscard;
  if (overflow_behavior_name == "block")
    return LoggerConfig::QueueOveflowBehavior::kBlock;
  throw std::runtime_error("Unknown queue overflow behavior '" +
                           overflow_behavior_name +
                           "' (must be one of 'block', 'discard')");
}

}  // namespace

const std::string LoggerConfig::kDefaultPattern =
    "tskv\ttimestamp=%Y-%m-%dT%H:%M:%S.%f\ttimezone=%z\tlevel=%l\t%v";

LoggerConfig LoggerConfig::ParseFromYaml(
    const formats::yaml::Value& yaml, const std::string& full_path,
    const yaml_config::VariableMapPtr& config_vars_ptr) {
  LoggerConfig config;
  config.file_path =
      yaml_config::ParseString(yaml, "file_path", full_path, config_vars_ptr);

  auto optional_level_str = yaml_config::ParseOptionalString(
      yaml, "level", full_path, config_vars_ptr);
  if (optional_level_str) config.level = LevelFromString(*optional_level_str);

  auto optional_pattern = yaml_config::ParseOptionalString(
      yaml, "pattern", full_path, config_vars_ptr);
  if (optional_pattern) config.pattern = std::move(*optional_pattern);

  auto optional_flush_level_str = yaml_config::ParseOptionalString(
      yaml, "flush_level", full_path, config_vars_ptr);
  if (optional_flush_level_str)
    config.flush_level = LevelFromString(*optional_flush_level_str);

  auto optional_message_queue_size = yaml_config::ParseOptionalUint64(
      yaml, "message_queue_size", full_path, config_vars_ptr);
  if (optional_message_queue_size)
    config.message_queue_size = *optional_message_queue_size;
  if (config.message_queue_size & (config.message_queue_size - 1)) {
    throw std::runtime_error("log message queue size must be a power of 2");
  }

  auto optional_overflow_behavior = yaml_config::ParseOptionalString(
      yaml, "overflow_behavior", full_path, config_vars_ptr);
  if (optional_overflow_behavior)
    config.queue_overflow_behavior =
        OverflowBehaviorFromString(*optional_overflow_behavior);

  auto optional_thread_pool_size = yaml_config::ParseOptionalUint64(
      yaml, "thread_pool_size", full_path, config_vars_ptr);
  if (optional_thread_pool_size)
    config.thread_pool_size = *optional_thread_pool_size;

  return config;
}

}  // namespace logging
