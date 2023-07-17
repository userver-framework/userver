#include "config.hpp"

#include <userver/logging/level_serialization.hpp>
#include <userver/utils/trivial_map.hpp>
#include <userver/yaml_config/yaml_config.hpp>

USERVER_NAMESPACE_BEGIN

namespace logging {

QueueOverflowBehavior Parse(const yaml_config::YamlConfig& value,
                            formats::parse::To<QueueOverflowBehavior>) {
  static constexpr utils::TrivialBiMap kMap([](auto selector) {
    return selector()
        .Case(QueueOverflowBehavior::kDiscard, "discard")
        .Case(QueueOverflowBehavior::kBlock, "block");
  });
  return utils::ParseFromValueString(value, kMap);
}

Format Parse(const yaml_config::YamlConfig& value, formats::parse::To<Format>) {
  const auto format_str = value.As<std::string>("tskv");
  return FormatFromString(format_str);
}

TestsuiteCaptureConfig Parse(const yaml_config::YamlConfig& value,
                             formats::parse::To<TestsuiteCaptureConfig>) {
  TestsuiteCaptureConfig config;
  config.host = value["host"].As<std::string>();
  config.port = value["port"].As<int>();
  return config;
}

void LoggerConfig::SetName(std::string name) { logger_name = std::move(name); }

LoggerConfig Parse(const yaml_config::YamlConfig& value,
                   formats::parse::To<LoggerConfig>) {
  LoggerConfig config;
  config.file_path = value["file_path"].As<std::string>();

  config.level = value["level"].As<logging::Level>(config.level);

  config.format = value["format"].As<Format>();

  config.flush_level =
      value["flush_level"].As<logging::Level>(config.flush_level);

  config.message_queue_size =
      value["message_queue_size"].As<size_t>(config.message_queue_size);

  config.queue_overflow_behavior =
      value["overflow_behavior"].As<QueueOverflowBehavior>(
          config.queue_overflow_behavior);

  config.fs_task_processor =
      value["fs-task-processor"].As<std::optional<std::string>>();

  config.testsuite_capture =
      value["testsuite-capture"].As<std::optional<TestsuiteCaptureConfig>>();

  return config;
}

}  // namespace logging

USERVER_NAMESPACE_END
