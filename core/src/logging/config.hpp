#pragma once

#include <string>
#include <unordered_map>

#include <userver/logging/format.hpp>
#include <userver/logging/level.hpp>
#include <userver/yaml_config/fwd.hpp>

USERVER_NAMESPACE_BEGIN

namespace logging {

struct TestsuiteCaptureConfig final {
  std::string host;
  int port{};
};

TestsuiteCaptureConfig Parse(const yaml_config::YamlConfig& value,
                             formats::parse::To<TestsuiteCaptureConfig>);

enum class QueueOverflowBehavior { kDiscard, kBlock };

QueueOverflowBehavior Parse(const yaml_config::YamlConfig& value,
                            formats::parse::To<QueueOverflowBehavior>);

struct LoggerConfig final {
  static constexpr size_t kDefaultMessageQueueSize = 1 << 16;

  void SetName(std::string name);

  std::string logger_name;

  std::string file_path;
  Level level = Level::kInfo;
  Format format = Format::kTskv;
  Level flush_level = Level::kWarning;

  // must be a power of 2
  size_t message_queue_size = kDefaultMessageQueueSize;
  QueueOverflowBehavior queue_overflow_behavior =
      QueueOverflowBehavior::kDiscard;

  std::optional<std::string> fs_task_processor;

  std::optional<TestsuiteCaptureConfig> testsuite_capture;
};

LoggerConfig Parse(const yaml_config::YamlConfig& value,
                   formats::parse::To<LoggerConfig>);

}  // namespace logging

USERVER_NAMESPACE_END
