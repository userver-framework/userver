#include <userver/server/handlers/tests_control.hpp>

#include <unordered_set>
#include <vector>

#include <userver/cache/update_type.hpp>
#include <userver/clients/http/component.hpp>
#include <userver/components/component.hpp>
#include <userver/components/statistics_storage.hpp>
#include <userver/logging/component.hpp>
#include <userver/logging/log.hpp>
#include <userver/server/http/http_error.hpp>
#include <userver/testsuite/http_testpoint_client.hpp>
#include <userver/testsuite/testsuite_support.hpp>
#include <userver/utils/datetime.hpp>
#include <userver/utils/mock_now.hpp>
#include <userver/yaml_config/merge_schemas.hpp>

USERVER_NAMESPACE_BEGIN

namespace server::handlers {

namespace {

cache::UpdateType ParseUpdateType(const formats::json::Value& value) {
  const auto update_type_string = value.As<std::string>();

  if (update_type_string == "full") {
    return cache::UpdateType::kFull;
  } else if (update_type_string == "incremental") {
    return cache::UpdateType::kIncremental;
  }

  LOG_ERROR() << "unknown update_type: " << update_type_string;
  throw ClientError();
}

}  // namespace

TestsControl::TestsControl(
    const components::ComponentConfig& config,
    const components::ComponentContext& component_context)
    : HttpHandlerJsonBase(config, component_context),
      testsuite_support_(
          component_context.FindComponent<components::TestsuiteSupport>()),
      metrics_storage_(
          component_context.FindComponent<components::StatisticsStorage>()
              .GetMetricsStorage()),
      logging_component_(
          component_context.FindComponent<components::Logging>()) {
  const bool skip_unregistered_testpoints =
      config["skip-unregistered-testpoints"].As<bool>(false);
  if (!skip_unregistered_testpoints) {
    testsuite_support_.GetTestpointControl().SetAllEnabled();
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
    testsuite_support_.GetTestpointControl().SetClient(*testpoint_client_);
  }
}

formats::json::Value TestsControl::HandleRequestJsonThrow(
    const http::HttpRequest& request, const formats::json::Value& request_body,
    request::RequestContext&) const {
  if (request.GetMethod() != http::HttpMethod::kPost) throw ClientError();

  // If the request object contains "action" field, then we only perform that
  // action, without normal configuration steps. The rest of the fields are
  // considered in the context of that action.
  if (request_body.HasMember("action")) {
    const auto action = request_body["action"].As<std::string>();
    if (action == "run_periodic_task") {
      return ActionRunPeriodicTask(request_body);
    } else if (action == "suspend_periodic_tasks") {
      return ActionSuspendPeriodicTasks(request_body);
    } else if (action == "write_cache_dumps") {
      return ActionWriteCacheDumps(request_body);
    } else if (action == "read_cache_dumps") {
      return ActionReadCacheDumps(request_body);
    }
    LOG_ERROR() << "unknown tests/control action " << action;
    throw ClientError();
  }

  const auto testpoints = request_body["testpoints"];
  if (!testpoints.IsMissing()) {
    if (!testpoint_client_) {
      LOG_ERROR() << "Trying to enable a testpoint, but testpoints are not "
                     "configured. Please provide 'testpoint-url' in the static "
                     "config of 'TestsControl' component.";
      throw ClientError();
    }
    testsuite_support_.GetTestpointControl().SetEnabledNames(
        testpoints.As<std::unordered_set<std::string>>());
  }

  if (request_body["reset_metrics"].As<bool>(false)) {
    metrics_storage_->ResetMetrics();
  }

  const auto mock_now = request_body["mock_now"];
  if (!mock_now.IsMissing()) {
    const auto now = mock_now.As<std::optional<std::string>>();
    if (now) {
      utils::datetime::MockNowSet(utils::datetime::Stringtime(*now));
    } else {
      utils::datetime::MockNowUnset();
    }
  }

  const auto socket_logging = request_body["socket_logging_duplication"];
  if (!socket_logging.IsMissing()) {
    auto enable = socket_logging.As<bool>();
    if (enable) {
      logging_component_.StartSocketLoggingDebug();
    } else {
      logging_component_.StopSocketLoggingDebug();
    }
  }

  const auto invalidate_caches = request_body["invalidate_caches"];
  if (!invalidate_caches.IsMissing()) {
    const auto update_type = ParseUpdateType(invalidate_caches["update_type"]);

    if (invalidate_caches.HasMember("names")) {
      testsuite_support_.GetCacheControl().InvalidateCaches(
          update_type,
          invalidate_caches["names"].As<std::unordered_set<std::string>>());
    } else {
      testsuite_support_.GetCacheControl().InvalidateAllCaches(
          update_type, invalidate_caches["names_blocklist"]
                           .As<std::unordered_set<std::string>>({}));
      testsuite_support_.GetComponentControl().InvalidateComponents();
    }
  }

  return {};
}

formats::json::Value TestsControl::ActionRunPeriodicTask(
    const formats::json::Value& request_body) const {
  const auto task_name = request_body["name"].As<std::string>();
  const bool status =
      testsuite_support_.GetPeriodicTaskControl().RunPeriodicTask(task_name);
  return formats::json::MakeObject("status", status);
}

formats::json::Value TestsControl::ActionSuspendPeriodicTasks(
    const formats::json::Value& request_body) const {
  const auto task_names =
      request_body["names"].As<std::unordered_set<std::string>>();
  testsuite_support_.GetPeriodicTaskControl().SuspendPeriodicTasks(task_names);
  return {};
}

formats::json::Value TestsControl::ActionWriteCacheDumps(
    const formats::json::Value& request_body) const {
  const auto dumper_names =
      request_body["names"].As<std::vector<std::string>>();
  testsuite_support_.GetDumpControl().WriteCacheDumps(dumper_names);
  return {};
}

formats::json::Value TestsControl::ActionReadCacheDumps(
    const formats::json::Value& request_body) const {
  const auto dumper_names =
      request_body["names"].As<std::vector<std::string>>();
  testsuite_support_.GetDumpControl().ReadCacheDumps(dumper_names);
  return {};
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
