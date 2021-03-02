#pragma once

#include <concurrent/variable.hpp>
#include <logging/level.hpp>
#include <server/handlers/http_handler_base.hpp>

namespace server {
namespace handlers {

class LogLevel final : public HttpHandlerBase {
 public:
  LogLevel(const components::ComponentConfig& config,
           const components::ComponentContext& component_context);

  static constexpr const char* kName = "handler-log-level";

  const std::string& HandlerName() const override;
  std::string HandleRequestThrow(const http::HttpRequest& request,
                                 request::RequestContext&) const override;

 private:
  std::string ProcessGet(const http::HttpRequest& request,
                         request::RequestContext&) const;
  std::string ProcessPut(const http::HttpRequest& request,
                         request::RequestContext&) const;

  components::Logging& logging_component_;
  struct Data {
    logging::Level default_init_level{logging::GetDefaultLoggerLevel()};
    mutable std::unordered_map<std::string, logging::Level> init_levels;
  };
  concurrent::Variable<Data> data_;
};

}  // namespace handlers
}  // namespace server
