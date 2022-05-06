#pragma once

/// @file userver/server/handlers/dynamic_debug_log.hpp
/// @brief HTTP Handler to show/hide logs at the specific file:line

#include <userver/concurrent/variable.hpp>
#include <userver/engine/task/task_processor_fwd.hpp>
#include <userver/logging/level.hpp>
#include <userver/server/handlers/http_handler_base.hpp>

USERVER_NAMESPACE_BEGIN

namespace server::handlers {
// clang-format off

/// @ingroup userver_components userver_http_handlers
///
/// @brief Handler for specific log lines enabling/disabling.
///
/// The component has no service configuration except the
/// @ref userver_http_handlers "common handler options".
///
/// ## Static configuration example:
///
/// @snippet components/common_server_component_list_test.cpp  Sample handler dynamic debug log component config

// clang-format on
class DynamicDebugLog final : public HttpHandlerBase {
 public:
  DynamicDebugLog(const components::ComponentConfig& config,
                  const components::ComponentContext& component_context);

  static constexpr std::string_view kName = "handler-dynamic-debug-log";

  std::string HandleRequestThrow(const http::HttpRequest& request,
                                 request::RequestContext&) const override;

  static yaml_config::Schema GetStaticConfigSchema();

 private:
  static std::string ProcessGet(const http::HttpRequest& request,
                                request::RequestContext&);
  static std::string ProcessPost(const http::HttpRequest& request,
                                 request::RequestContext&);
};

}  // namespace server::handlers

template <>
inline constexpr bool
    components::kHasValidate<server::handlers::DynamicDebugLog> = true;

USERVER_NAMESPACE_END
