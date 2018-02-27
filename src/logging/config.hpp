#pragma once

#include <string>
#include <unordered_map>

#include <json/value.h>

#include <json_config/variable_map.hpp>

#include "level.hpp"

namespace logging {

struct LoggerConfig {
  static const std::string kDefaultPattern;

  std::string file_path;
  Level level = Level::kInfo;
  std::string pattern = kDefaultPattern;
  Level flush_level = Level::kWarning;

  static LoggerConfig ParseFromJson(
      const Json::Value& json, const std::string& full_path,
      const json_config::VariableMapPtr& config_vars_ptr);
};

struct Config {
  static constexpr size_t kDefaultMessageQueueSize = 1 << 16;

  enum class QueueOveflowBehavior { kDiscard, kBlock };

  // per-logger, must be a power of 2
  size_t message_queue_size = kDefaultMessageQueueSize;
  QueueOveflowBehavior queue_overflow_behavior = QueueOveflowBehavior::kDiscard;

  std::unordered_map<std::string, LoggerConfig> logger_configs;

  static Config ParseFromJson(
      const Json::Value& json, const std::string& full_path,
      const json_config::VariableMapPtr& config_vars_ptr);
};

}  // namespace logging
