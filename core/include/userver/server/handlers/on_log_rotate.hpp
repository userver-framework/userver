#pragma once

/// @file userver/server/handlers/on_log_rotate.hpp
/// @brief @copybrief server::handlers::OnLogRotate

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
/// POST request reopens log file for all loggers.
/// Returns 200 status code after successful operation.
/// If at least one of files was not successfully reopened returns 500 status
/// code and error messages separated by comma in response body.

// clang-format on
class OnLogRotate final : public HttpHandlerBase {
 public:
  OnLogRotate(const components::ComponentConfig& config,
              const components::ComponentContext& component_context);

  /// @ingroup userver_component_names
  /// @brief The default name of server::handlers::OnLogRotate
  static constexpr std::string_view kName = "handler-on-log-rotate";

  std::string HandleRequestThrow(const http::HttpRequest& request,
                                 request::RequestContext&) const override;

  static yaml_config::Schema GetStaticConfigSchema();

 private:
  components::Logging& logging_component_;
};

}  // namespace server::handlers

template <>
inline constexpr bool components::kHasValidate<server::handlers::OnLogRotate> =
    true;

USERVER_NAMESPACE_END
