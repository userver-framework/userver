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
/// @brief Handler for forcing specific lines logging. Feature also known as
/// dynamic debug logging.
///
/// The component has no service configuration except the
/// @ref userver_http_handlers "common handler options".
///
/// ## Static configuration example:
///
/// @snippet components/common_server_component_list_test.cpp  Sample handler dynamic debug log component config
///
/// ## Scheme
///
/// `GET` request shows information about available locations and forced
/// loggings. `PUT` adds forced logging for a particular location. `DELETE`
/// request removes forced logging from a location.
///
/// ### GET
/// For the `GET` requests this handler returns a list of all the known logging
/// locations tab separated from a on/off value of logging:
/// @code
/// project/src/some.cpp:13     0
/// project/src/some.cpp:23     0
/// project/src/some.cpp:42     1
/// project/src/some.cpp:113    0
/// userver/core/src/server/server.cpp:131    0
/// @endcode
///
/// In the above sample `1` means that logging was enabled via this handler
/// and that logger would write logs even if the logger level tells not
/// to do that. `0` means that the log will be written down only if the logger
/// level tells to do that.
///
/// ### PUT
/// `PUT` request enables logging for the location specified in a `location=`
/// argument in URL. `PUT` request should have a `location=` argument in URL
/// with a location from the `GET` request or with a location without line
/// number, to enable logging for the whole file.
///
/// ### DELETE
/// `DELETE` request removes the forced logging from location. Location should
/// be specified in the `location=` argument in URL.
///
/// @see @ref md_en_userver_log_level_running_service

// clang-format on
class DynamicDebugLog final : public HttpHandlerBase {
 public:
  DynamicDebugLog(const components::ComponentConfig& config,
                  const components::ComponentContext& component_context);

  /// @ingroup userver_component_names
  /// @brief The default name of server::handlers::DynamicDebugLog
  static constexpr std::string_view kName = "handler-dynamic-debug-log";

  std::string HandleRequestThrow(const http::HttpRequest& request,
                                 request::RequestContext&) const override;

  static yaml_config::Schema GetStaticConfigSchema();
};

}  // namespace server::handlers

template <>
inline constexpr bool
    components::kHasValidate<server::handlers::DynamicDebugLog> = true;

USERVER_NAMESPACE_END
