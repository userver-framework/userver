#pragma once

/// @file userver/server/handlers/log_level.hpp
/// @brief @copybrief server::handlers::LogLevel

#include <userver/concurrent/variable.hpp>
#include <userver/logging/level.hpp>
#include <userver/server/handlers/http_handler_base.hpp>

USERVER_NAMESPACE_BEGIN

namespace components {
class Logging;
}  // namespace components

namespace server::handlers {
// clang-format off

/// @ingroup userver_components userver_http_handlers
///
/// @brief Handler that controls logging levels of all the loggers.
///
/// The component has no service configuration except the
/// @ref userver_http_handlers "common handler options".
///
/// ## Static configuration example:
///
/// @snippet components/common_server_component_list_test.cpp  Sample handler log level component config
///
/// ## Scheme
/// For the GET and PUT requests this handler returns the following JSON:
/// @code
/// {
///   "init-log-level": <log level on service start>,
///   "current-log-level": <current log level>
/// }
/// @endcode
///
/// Particular logger name could be specified by an optional `logger` query
/// argument. Default logger is used, if no `logger` was provided.
///
/// PUT request changes the logger level to the value specified in the `level`
/// query argument. Set it to the `reset` value, to reset the logger level to
/// the initial values.
///
/// @see @ref md_en_userver_log_level_running_service

// clang-format on
class LogLevel final : public HttpHandlerBase {
 public:
  LogLevel(const components::ComponentConfig& config,
           const components::ComponentContext& component_context);

  /// @ingroup userver_component_names
  /// @brief The default name of server::handlers::LogLevel
  static constexpr std::string_view kName = "handler-log-level";

  std::string HandleRequestThrow(const http::HttpRequest& request,
                                 request::RequestContext&) const override;

  static yaml_config::Schema GetStaticConfigSchema();

 private:
  std::string ProcessGet(const http::HttpRequest& request,
                         request::RequestContext&) const;
  std::string ProcessPut(const http::HttpRequest& request,
                         request::RequestContext&) const;

  components::Logging& logging_component_;
  struct Data {
    logging::Level default_init_level;
    mutable std::unordered_map<std::string, logging::Level> init_levels;
  };
  concurrent::Variable<Data> data_;
};

}  // namespace server::handlers

template <>
inline constexpr bool components::kHasValidate<server::handlers::LogLevel> =
    true;

USERVER_NAMESPACE_END
