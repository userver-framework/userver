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
  static constexpr size_t kDefaultThreadPoolSize = 1;
  static constexpr auto kDefaultTskvPattern =
      "tskv\ttimestamp=%Y-%m-%dT%H:%M:%S.%f\tlevel=%l\t%v";
  static constexpr auto kDefaultLtsvPattern =
      "timestamp:%Y-%m-%dT%H:%M:%S.%f\tlevel:%l\t%v";

  enum class QueueOveflowBehavior { kDiscard, kBlock };

  std::string file_path;
  Level level = Level::kInfo;
  Format format = Format::kTskv;
  std::string pattern = kDefaultTskvPattern;
  Level flush_level = Level::kWarning;

  // must be a power of 2
  size_t message_queue_size = kDefaultMessageQueueSize;
  QueueOveflowBehavior queue_overflow_behavior = QueueOveflowBehavior::kDiscard;

  size_t thread_pool_size = kDefaultThreadPoolSize;
};

LoggerConfig Parse(const yaml_config::YamlConfig& value,
                   formats::parse::To<LoggerConfig>);

}  // namespace logging

USERVER_NAMESPACE_END
