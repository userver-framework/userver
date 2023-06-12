#include <userver/server/handlers/tests_control.hpp>

#include <userver/clients/http/component.hpp>
#include <userver/components/component.hpp>
#include <userver/server/http/http_error.hpp>
#include <userver/testsuite/testsuite_support.hpp>
#include <userver/yaml_config/merge_schemas.hpp>

#include <testsuite/impl/actions/caches.hpp>
#include <testsuite/impl/actions/control.hpp>
#include <testsuite/impl/actions/http_allowed_urls_extra.hpp>
#include <testsuite/impl/actions/logcapture.hpp>
#include <testsuite/impl/actions/metrics_portability.hpp>
#include <testsuite/impl/actions/periodic.hpp>
#include <testsuite/impl/actions/reset_metrics.hpp>
#include <testsuite/impl/actions/tasks.hpp>

USERVER_NAMESPACE_BEGIN

namespace server::handlers {

namespace {
const std::string kDefaultAction = "control";
}

TestsControl::TestsControl(
    const components::ComponentConfig& config,
    const components::ComponentContext& component_context)
    : HttpHandlerJsonBase(config, component_context) {
  auto& testsuite_support =
      component_context.FindComponent<components::TestsuiteSupport>();

  const bool skip_unregistered_testpoints =
      config["skip-unregistered-testpoints"].As<bool>(false);
  if (!skip_unregistered_testpoints) {
    testsuite_support.GetTestpointControl().SetAllEnabled();
  }

  const auto testpoint_url =
      config["testpoint-url"].As<std::optional<std::string>>();

  if (testpoint_url) {
    auto& http_client =
        component_context.FindComponent<components::HttpClient>()
            .GetHttpClient();
    const auto testpoint_timeout =
        config["testpoint-timeout"].As<std::chrono::milliseconds>(
            std::chrono::seconds(1));
    testpoint_client_ = std::make_unique<testsuite::impl::HttpTestpointClient>(
        http_client, *testpoint_url, testpoint_timeout);
    testsuite_support.GetTestpointControl().SetClient(*testpoint_client_);
  }

  namespace actions = testsuite::impl::actions;

  // Default tests/control action
  actions_.emplace(kDefaultAction,
                   std::make_unique<actions::Control>(
                       component_context, testpoint_client_ != nullptr));

  // Periodic tasks support
  actions_.emplace(
      "run_periodic_task",
      std::make_unique<actions::PeriodicTaskRun>(testsuite_support));
  actions_.emplace(
      "suspend_periodic_tasks",
      std::make_unique<actions::PeriodicTaskSuspend>(testsuite_support));

  // Cache support
  actions_.emplace(
      "write_cache_dumps",
      std::make_unique<actions::CacheDumpsWrite>(testsuite_support));
  actions_.emplace(
      "read_cache_dumps",
      std::make_unique<actions::CacheDumpsRead>(testsuite_support));

  // Metrics
  actions_.emplace("reset_metrics",
                   std::make_unique<actions::ResetMetrics>(component_context));
  actions_.emplace(
      "metrics_portability",
      std::make_unique<actions::MetricsPortability>(component_context));

  // Log capture
  actions_.emplace("log_capture",
                   std::make_unique<actions::LogCapture>(component_context));

  // Testsuite tasks support
  actions_.emplace("task_run",
                   std::make_unique<actions::TaskRun>(testsuite_support));
  actions_.emplace("task_spawn",
                   std::make_unique<actions::TaskSpawn>(testsuite_support));
  actions_.emplace("task_stop",
                   std::make_unique<actions::TaskStop>(testsuite_support));
  actions_.emplace("tasks_list",
                   std::make_unique<actions::TasksList>(testsuite_support));

  // HTTP client
  actions_.emplace(
      "http_allowed_urls_extra",
      std::make_unique<actions::HttpAllowedUrlsExtra>(testsuite_support));
}

TestsControl::~TestsControl() = default;

formats::json::Value TestsControl::HandleRequestJsonThrow(
    const http::HttpRequest& request, const formats::json::Value& request_body,
    request::RequestContext&) const {
  if (request.GetMethod() != http::HttpMethod::kPost) throw ClientError();

  const auto& path_action = request.GetPathArg(0);
  if (!path_action.empty() && path_action != kDefaultAction) {
    return PerformAction(path_action, request_body);
  }

  // If the request object contains "action" field, then we only perform that
  // action, without normal configuration steps. The rest of the fields are
  // considered in the context of that action.
  if (request_body.HasMember("action")) {
    const auto action_name = request_body["action"].As<std::string>();
    return PerformAction(action_name, request_body);
  }

  return PerformAction(kDefaultAction, request_body);
}

formats::json::Value TestsControl::PerformAction(
    const std::string& action_name,
    const formats::json::Value& request_body) const {
  const auto it = actions_.find(action_name);
  if (it != actions_.end()) {
    return it->second->Perform(request_body);
  }
  LOG_ERROR() << "unknown tests/control action " << action_name;
  throw ClientError();
}

yaml_config::Schema TestsControl::GetStaticConfigSchema() {
  return yaml_config::MergeSchemas<HttpHandlerJsonBase>(R"(
type: object
description: tests-control config
additionalProperties: false
properties:
    testpoint-url:
        type: string
        description: an URL that should be notified in the TESTPOINT_CALLBACK and TESTPOINT_CALLBACK_NONCORO macros
    skip-unregistered-testpoints:
        type: boolean
        description: do not send testpoints data for paths that were not registered by `testpoint-url`
        defaultDescription: false
    testpoint-timeout:
        type: string
        description: timeout to use while working with testpoint-url
        defaultDescription: 1s
)");
}

}  // namespace server::handlers

USERVER_NAMESPACE_END
