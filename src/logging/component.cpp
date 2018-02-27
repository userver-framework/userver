#include "component.hpp"

#include <chrono>
#include <stdexcept>

#include <spdlog/common.h>
#include <spdlog/details/log_msg.h>

#include "config.hpp"
#include "log.hpp"
#include "logger.hpp"

namespace components {
namespace {

std::chrono::seconds kDefaultFlushInterval{2};

}  // namespace

Logging::Logging(const ComponentConfig& config, const ComponentContext&) {
  auto logging_config = logging::Config::ParseFromJson(
      config.Json(), config.FullPath(), config.ConfigVarsPtr());

  auto overflow_policy = spdlog::async_overflow_policy::discard_log_msg;
  if (logging_config.queue_overflow_behavior ==
      logging::Config::QueueOveflowBehavior::kBlock) {
    overflow_policy = spdlog::async_overflow_policy::block_retry;
  }
  spdlog::set_async_mode(logging_config.message_queue_size, overflow_policy,
                         nullptr, kDefaultFlushInterval);

  for (auto& item : logging_config.logger_configs) {
    auto& name = item.first;
    const auto& logger_config = item.second;

    auto logger = logging::MakeFileLogger(name, logger_config.file_path);
    logger->set_level(
        static_cast<spdlog::level::level_enum>(logger_config.level));
    logger->set_pattern(logger_config.pattern);
    logger->flush_on(
        static_cast<spdlog::level::level_enum>(logger_config.flush_level));

    if (name == "default") {
      logging::Log() = std::move(logger);
    } else {
      loggers_.emplace(std::move(name), std::move(logger));
    }
  }
}

logging::LoggerPtr Logging::GetLogger(const std::string& name) {
  auto it = loggers_.find(name);
  if (it == loggers_.end()) {
    throw std::runtime_error("logger '" + name + "' not found");
  }
  return it->second;
}

void Logging::OnLogRotate() {
  spdlog::details::log_msg rotate_msg;
  rotate_msg.level = spdlog::level::off;  // covers all log levels
  rotate_msg.rotate_only = true;

  for (auto& item : loggers_) {
    const auto& name = item.first;
    auto& logger = item.second;

    rotate_msg.logger_name = &name;
    logger->_sink_it(rotate_msg);
  }
}

}  // namespace components
