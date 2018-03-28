#include "component.hpp"

#include <chrono>
#include <stdexcept>

#include <spdlog/async_logger.h>
#include <spdlog/common.h>
#include <spdlog/details/log_msg.h>
#include <spdlog/sinks/file_sinks.h>

#include <json_config/value.hpp>

#include "config.hpp"
#include "log.hpp"
#include "logger.hpp"

namespace components {
namespace {

std::chrono::seconds kDefaultFlushInterval{2};

}  // namespace

Logging::Logging(const ComponentConfig& config, const ComponentContext&) {
  auto loggers = config.Json()["loggers"];
  auto loggers_full_path = config.FullPath() + ".loggers";
  json_config::CheckIsObject(loggers, loggers_full_path);

  for (auto it = loggers.begin(); it != loggers.end(); ++it) {
    auto logger_name = it.key().asString();
    auto logger_full_path = loggers_full_path + '.' + logger_name;

    auto logger_config = logging::LoggerConfig::ParseFromJson(
        *it, logger_full_path, config.ConfigVarsPtr());

    auto overflow_policy = spdlog::async_overflow_policy::discard_log_msg;
    if (logger_config.queue_overflow_behavior ==
        logging::LoggerConfig::QueueOveflowBehavior::kBlock) {
      overflow_policy = spdlog::async_overflow_policy::block_retry;
    }

    auto file_sink = std::make_shared<spdlog::sinks::rotating_file_sink_mt>(
        logger_config.file_path, /*max_size=*/-1,
        /*max_files=*/0);
    auto logger = std::make_shared<spdlog::async_logger>(
        logger_name, std::move(file_sink), logger_config.message_queue_size,
        overflow_policy, nullptr, kDefaultFlushInterval);
    logger->set_level(
        static_cast<spdlog::level::level_enum>(logger_config.level));
    logger->set_pattern(logger_config.pattern);
    logger->flush_on(
        static_cast<spdlog::level::level_enum>(logger_config.flush_level));

    if (logger_name == "default") {
      logging::Log() = std::move(logger);
    } else {
      auto insertion_result =
          loggers_.emplace(std::move(logger_name), std::move(logger));
      if (!insertion_result.second) {
        throw std::runtime_error("duplicate logger '" +
                                 insertion_result.first->first + '\'');
      }
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

  auto default_logger = logging::Log();
  rotate_msg.logger_name = &default_logger->name();
  default_logger->_sink_it(rotate_msg);
  
  for (auto& item : loggers_) {
    const auto& name = item.first;
    auto& logger = item.second;

    rotate_msg.logger_name = &name;
    logger->_sink_it(rotate_msg);
  }
}

}  // namespace components
