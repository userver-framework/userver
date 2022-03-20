#include <userver/server/handlers/log_level.hpp>

#include <userver/engine/mutex.hpp>
#include <userver/formats/json/serialize.hpp>
#include <userver/logging/log.hpp>
#include <userver/utils/algo.hpp>

USERVER_NAMESPACE_BEGIN

namespace server::handlers {
namespace {

const std::string kLogger = "logger";
const std::string kLevel = "level";
const std::string kReset = "reset";

}  // namespace

LogLevel::LogLevel(const components::ComponentConfig& config,
                   const components::ComponentContext& context)
    : HttpHandlerBase(config, context, /*is_monitor = */ true),
      logging_component_(context.FindComponent<components::Logging>()) {}

std::string LogLevel::HandleRequestThrow(
    const http::HttpRequest& request, request::RequestContext& context) const {
  switch (request.GetMethod()) {
    case http::HttpMethod::kGet:
      return ProcessGet(request, context);
    case http::HttpMethod::kPut:
      return ProcessPut(request, context);
    default:
      throw std::runtime_error("unsupported method: " + request.GetMethodStr());
  }
}

std::string LogLevel::ProcessGet(const http::HttpRequest& request,
                                 request::RequestContext&) const {
  const std::string& level_arg = request.GetPathArg(kLevel);
  if (!level_arg.empty()) {
    std::string message =
        std::string{"unexpected parameter for log level getting: " + level_arg};
    throw ClientError(InternalMessage{message}, ExternalBody{message});
  }
  const std::string& logger_name = request.GetArg(kLogger);

  formats::json::ValueBuilder response;
  auto data = data_.Lock();
  if (logger_name.empty()) {
    // default
    response["init-log-level"] = ToString(data->default_init_level);
    response["current-log-level"] = ToString(logging::GetDefaultLoggerLevel());
  } else {
    const auto& logger = logging_component_.GetLogger(logger_name);

    auto current_level = logging::GetLoggerLevel(logger);
    auto [it, _] = data->init_levels.emplace(logger_name, current_level);
    response["init-log-level"] = ToString(it->second);
    response["current-log-level"] = ToString(current_level);
  }

  return formats::json::ToString(response.ExtractValue()) + "\n";
}

std::string LogLevel::ProcessPut(const http::HttpRequest& request,
                                 request::RequestContext&) const {
  const std::string& logger_name = request.GetArg(kLogger);
  const logging::LoggerPtr logger =
      logger_name.empty() ? nullptr : logging_component_.GetLogger(logger_name);
  auto data = data_.Lock();

  logging::Level init_level;
  if (logger_name.empty()) {
    init_level = data->default_init_level;
  } else {
    auto current_level = logging::GetLoggerLevel(logger);
    auto [it, _] = data->init_levels.emplace(logger_name, current_level);
    init_level = it->second;
  }

  const std::string& level_arg = request.GetPathArg(kLevel);
  logging::Level level;
  if (level_arg == kReset) {
    level = init_level;
  } else {
    level = logging::LevelFromString(level_arg);
  }

  if (logger_name.empty()) {
    logging::SetDefaultLoggerLevel(level);
  } else {
    logging::SetLoggerLevel(logger, level);
  }

  formats::json::ValueBuilder response;
  response["init-log-level"] = ToString(init_level);
  response["current-log-level"] = ToString(level);

  return formats::json::ToString(response.ExtractValue()) + "\n";
}
yaml_config::Schema LogLevel::GetStaticConfigSchema() {
  auto schema = HttpHandlerBase::GetStaticConfigSchema();
  schema.UpdateDescription("handler-log-level config");
  return schema;
}

}  // namespace server::handlers

USERVER_NAMESPACE_END
