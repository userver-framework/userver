#include <server/handlers/log_level.hpp>

#include <engine/mutex.hpp>
#include <formats/json/serialize.hpp>
#include <logging/log.hpp>

namespace server::handlers {
namespace {

const std::string kLevel = "level";
const std::string kReset = "reset";

}  // namespace

LogLevel::LogLevel(const components::ComponentConfig& config,
                   const components::ComponentContext& component_context)
    : HttpHandlerBase(config, component_context, /*is_monitor = */ true),
      init_level_(logging::GetDefaultLoggerLevel()) {}

const std::string& LogLevel::HandlerName() const {
  static const std::string kHandlerName = kName;
  return kHandlerName;
}

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

  formats::json::ValueBuilder response;
  response["init-log-level"] = ToString(init_level_);
  response["current-log-level"] = ToString(logging::GetDefaultLoggerLevel());

  return formats::json::ToString(response.ExtractValue());
}

std::string LogLevel::ProcessPut(const http::HttpRequest& request,
                                 request::RequestContext&) const {
  logging::Level level;
  const std::string& level_arg = request.GetPathArg(kLevel);
  if (level_arg == kReset) {
    level = init_level_;
  } else {
    try {
      level = logging::LevelFromString(level_arg);
    } catch (const std::exception& ex) {
      std::string message =
          std::string{"invalid or missing parameter in request path: "} +
          ex.what();
      throw ClientError(InternalMessage{message}, ExternalBody{message});
    }
  }
  logging::SetDefaultLoggerLevel(level);

  formats::json::ValueBuilder response;
  response["init-log-level"] = ToString(init_level_);
  response["current-log-level"] = ToString(level);

  return formats::json::ToString(response.ExtractValue());
}

}  // namespace server::handlers
