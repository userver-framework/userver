#pragma once

#include <string>
#include <unordered_map>

#include <userver/formats/yaml.hpp>
#include <userver/logging/format.hpp>
#include <userver/logging/level.hpp>
#include <userver/yaml_config/fwd.hpp>

USERVER_NAMESPACE_BEGIN

namespace logging {

struct LoggerConfig {
  static constexpr size_t kDefaultMessageQueueSize = 1 << 16;

  enum class QueueOverflowBehavior { kDiscard, kBlock };

  std::string file_path;
  Level level = Level::kInfo;
  Format format = Format::kTskv;
  std::string pattern;  // deprecated
  Level flush_level = Level::kWarning;

  // must be a power of 2
  size_t message_queue_size = kDefaultMessageQueueSize;
  QueueOverflowBehavior queue_overflow_behavior =
      QueueOverflowBehavior::kDiscard;

  std::optional<std::string> fs_task_processor;
};

LoggerConfig Parse(const yaml_config::YamlConfig& value,
                   formats::parse::To<LoggerConfig>);

}  // namespace logging

USERVER_NAMESPACE_END
