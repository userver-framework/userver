#pragma once

/// @file userver/server/handlers/tests_control.hpp
/// @brief @copybrief server::handlers::TestsControl

#include <functional>
#include <vector>

#include <userver/cache/cache_update_trait.hpp>
#include <userver/server/handlers/http_handler_json_base.hpp>

USERVER_NAMESPACE_BEGIN

namespace components {
class TestsuiteSupport;
}  // namespace components

namespace server::handlers {

// clang-format off

/// @ingroup userver_components userver_http_handlers
///
/// @brief Handler that allows to control the behavior of server from tests.
///
/// It is highly recommended to disable this handle in production via the
/// @ref userver_components "load-enabled: false" option.
///
/// The component must be configured in service config.
///
/// ## Static options:
/// Aside from @ref userver_http_handlers "common handler options" component
/// has the following options:
///
/// Name | Description | Default value
/// ---- | ----------- | -------------
/// testpoint-url | an URL that should be notified in the TESTPOINT_CALLBACK and TESTPOINT_CALLBACK_NONCORO macros | -
/// skip-unregistered-testpoints | do not send tespoints data for paths that were not registered by `testpoint-url` | false
/// testpoint-timeout | timeout to use while working with testpoint-url | 1s
///
/// ## Static configuration example:
///
/// @snippet components/common_server_component_list_test.cpp  Sample tests control component config
///
/// ## Scheme
/// The scheme matches the https://yandex.github.io/yandex-taxi-testsuite/
/// expectations from `/tests/control` handle. In particular:
/// @code
/// {
///     "action": "run_periodic_task" | "suspend_periodic_tasks" | "write_cache_dumps" | "read_cache_dumps"
///     "testpoints": [<list of testpoints to register>]
///     "reset_metrics": true | false
///     "mock_now": <time in utils::datetime::Stringtime() acceptable format>
///     "invalidate_caches": <...>
///     "socket_logging_duplication": true | false
///     <...>
/// }
/// @endcode

// clang-format on
class TestsControl final : public HttpHandlerJsonBase {
 public:
  TestsControl(const components::ComponentConfig& config,
               const components::ComponentContext& component_context);

  static constexpr std::string_view kName = "tests-control";

  formats::json::Value HandleRequestJsonThrow(
      const http::HttpRequest& request,
      const formats::json::Value& request_body,
      request::RequestContext& context) const override;

 private:
  formats::json::Value ActionRunPeriodicTask(
      const formats::json::Value& request_body) const;
  formats::json::Value ActionSuspendPeriodicTasks(
      const formats::json::Value& request_body) const;
  formats::json::Value ActionWriteCacheDumps(
      const formats::json::Value& request_body) const;
  formats::json::Value ActionReadCacheDumps(
      const formats::json::Value& request_body) const;

  components::TestsuiteSupport& testsuite_support_;
  components::Logging& logging_component_;
};

}  // namespace server::handlers

USERVER_NAMESPACE_END
