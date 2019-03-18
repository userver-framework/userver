#pragma once

#include <string>
#include <unordered_map>

#include <formats/yaml.hpp>

#include <logging/level.hpp>
#include <yaml_config/variable_map.hpp>

namespace logging {

struct LoggerConfig {
  static constexpr size_t kDefaultMessageQueueSize = 1 << 16;
  static constexpr size_t kDefaultThreadPoolSize = 1;
  static const std::string kDefaultPattern;

  enum class QueueOveflowBehavior { kDiscard, kBlock };

  std::string file_path;
  Level level = Level::kInfo;
  std::string pattern = kDefaultPattern;
  Level flush_level = Level::kWarning;

  // must be a power of 2
  size_t message_queue_size = kDefaultMessageQueueSize;
  QueueOveflowBehavior queue_overflow_behavior = QueueOveflowBehavior::kDiscard;

  size_t thread_pool_size = kDefaultThreadPoolSize;

  static LoggerConfig ParseFromYaml(
      const formats::yaml::Value& yaml, const std::string& full_path,
      const yaml_config::VariableMapPtr& config_vars_ptr);
};

}  // namespace logging
