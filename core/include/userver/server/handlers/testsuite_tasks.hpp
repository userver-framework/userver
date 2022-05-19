#pragma once

/// @file userver/server/handlers/testsuite_tasks.hpp
/// @brief @copybrief server::handlers::TestsuiteTasks

#include <string>

#include <userver/server/handlers/http_handler_json_base.hpp>

USERVER_NAMESPACE_BEGIN

namespace testsuite {
class TestsuiteTasks;
}

namespace server::handlers {

// clang-format off

/// @ingroup userver_components userver_http_handlers
///
/// @brief Handler that allows testsuite to access testsuite-tasks facility.
///
/// This handler provides access to testsuite::TestsuiteTasks for testsuite.
///
/// It is highly recommended to disable this handler in production via the
/// @ref userver_components "load-enabled: false" option.
///
/// The component has no service configuration except the
/// @ref userver_http_handlers "common handler options".
///
/// ## Static configuration example:
///
/// @snippet samples/testsuite-support/static_config.yaml Testsuite tasks
///
/// ## Scheme (sample requests)
/// ### Run task
/// @code
/// {
///     "action": "run",
///     "name": "task-name"
/// }
/// @endcode
///
/// ### Spawn task
/// @code
/// {
///     "action": "spawn",
///     "name": "task-name"
/// }
/// @endcode
///
/// ### Stop task
/// @code
/// {
///     "action": "stop",
///     "task_id": "task_id"
/// }
/// @endcode

// clang-format on
class TestsuiteTasks final : public server::handlers::HttpHandlerJsonBase {
 public:
  static constexpr std::string_view kName = "handler-testsuite-tasks";

  TestsuiteTasks(const components::ComponentConfig&,
                 const components::ComponentContext&);

  formats::json::Value HandleRequestJsonThrow(
      const server::http::HttpRequest& request,
      const formats::json::Value& request_body,
      server::request::RequestContext& context) const override;

 private:
  formats::json::Value ActionRun(
      const formats::json::Value& request_body) const;
  formats::json::Value ActionSpawn(
      const formats::json::Value& request_body) const;
  formats::json::Value ActionStop(
      const formats::json::Value& request_body) const;

  testsuite::TestsuiteTasks& tasks_;
};

}  // namespace server::handlers

USERVER_NAMESPACE_END
