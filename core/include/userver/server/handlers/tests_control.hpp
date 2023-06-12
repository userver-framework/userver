#pragma once

/// @file userver/server/handlers/tests_control.hpp
/// @brief @copybrief server::handlers::TestsControl

#include <memory>

#include <userver/server/handlers/http_handler_json_base.hpp>
#include <userver/testsuite/http_testpoint_client.hpp>

USERVER_NAMESPACE_BEGIN

namespace testsuite::impl::actions {
class BaseTestsuiteAction;
}

namespace server::handlers {

// clang-format off

/// @ingroup userver_components userver_http_handlers
///
/// @brief Handler that allows to control the behavior of server from tests,
/// and @ref md_en_userver_functional_testing "functional tests with testsuite"
/// in particular.
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
/// skip-unregistered-testpoints | do not send testpoints data for paths that were not registered by `testpoint-url` | false
/// testpoint-timeout | timeout to use while working with testpoint-url | 1s
///
/// ## Static configuration example:
///
/// @snippet components/common_server_component_list_test.cpp  Sample tests control component config
///
/// ## Scheme
/// Main user of the scheme is the pytest_userver.client.Client python class.
/// In particular:
/// @code
/// {
///     "action": "run_periodic_task" | "suspend_periodic_tasks" | "write_cache_dumps" | "read_cache_dumps" | "metrics_portability"
///     "testpoints": [<list of testpoints to register>]
///     "reset_metrics": true | false
///     "mock_now": <time in utils::datetime::Stringtime() acceptable format>
///     "invalidate_caches": <...>
///     "socket_logging_duplication": true | false
///     <...>
/// }
/// @endcode
///
/// @see @ref md_en_userver_functional_testing

// clang-format on
class TestsControl final : public HttpHandlerJsonBase {
 public:
  TestsControl(const components::ComponentConfig& config,
               const components::ComponentContext& component_context);
  ~TestsControl() override;

  /// @ingroup userver_component_names
  /// @brief The default name of server::handlers::TestsControl
  static constexpr std::string_view kName = "tests-control";

  formats::json::Value HandleRequestJsonThrow(
      const http::HttpRequest& request,
      const formats::json::Value& request_body,
      request::RequestContext& context) const override;

  static yaml_config::Schema GetStaticConfigSchema();

 private:
  formats::json::Value PerformAction(
      const std::string& action_name,
      const formats::json::Value& request_body) const;

  std::unique_ptr<testsuite::TestpointClientBase> testpoint_client_;
  std::unordered_map<
      std::string,
      std::unique_ptr<testsuite::impl::actions::BaseTestsuiteAction>>
      actions_;
};

}  // namespace server::handlers

USERVER_NAMESPACE_END
