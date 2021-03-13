#include <server/handlers/tests_control.hpp>

#include <unordered_set>

#include <cache/update_type.hpp>
#include <clients/http/component.hpp>
#include <logging/log.hpp>
#include <server/http/http_error.hpp>
#include <testsuite/testpoint.hpp>
#include <testsuite/testsuite_support.hpp>
#include <utils/datetime.hpp>
#include <utils/mock_now.hpp>

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
          component_context.FindComponent<components::TestsuiteSupport>()) {
  const auto testpoint_url =
      config["testpoint-url"].As<std::optional<std::string>>();
  const auto skip_unregistered_testpoints =
      config["skip-unregistered-testpoints"].As<bool>(false);

  if (testpoint_url) {
    auto& http_client =
        component_context.FindComponent<components::HttpClient>()
            .GetHttpClient();
    const auto testpoint_timeout =
        config["testpoint-timeout"].As<std::chrono::milliseconds>(
            std::chrono::seconds(1));
    auto& tp = testsuite::impl::TestPoint::GetInstance();
    tp.Setup(http_client, *testpoint_url, testpoint_timeout,
             skip_unregistered_testpoints);
  }
}

const std::string& TestsControl::HandlerName() const {
  static const std::string kTestsControlName = kName;
  return kTestsControlName;
}

formats::json::Value TestsControl::HandleRequestJsonThrow(
    const http::HttpRequest& request, const formats::json::Value& request_body,
    request::RequestContext&) const {
  if (request.GetMethod() != http::HttpMethod::kPost) throw ClientError();

  const auto testpoints = request_body["testpoints"];
  if (!testpoints.IsMissing()) {
    auto& tp = ::testsuite::impl::TestPoint::GetInstance();
    tp.RegisterPaths(testpoints.As<std::vector<std::string>>());
  }

  if (request_body.HasMember("action")) {
    const auto action = request_body["action"].As<std::string>();
    if (action == "run_periodic_task") {
      return ActionRunPeriodicTask(request_body);
    } else if (action == "suspend_periodic_tasks") {
      return ActionSuspendPeriodicTasks(request_body);
    }
    LOG_ERROR() << "unknown tests/control action " << action;
    throw ClientError();
  }

  auto testsuite_support = testsuite_support_.Lock();

  if (request_body["reset_metrics"].As<bool>(false)) {
    testsuite_support->get().ResetMetrics();
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

  const auto invalidate_caches = request_body["invalidate_caches"];
  if (!invalidate_caches.IsMissing()) {
    const auto update_type = ParseUpdateType(invalidate_caches["update_type"]);

    if (invalidate_caches.HasMember("names")) {
      testsuite_support->get().InvalidateCaches(
          update_type,
          invalidate_caches["names"].As<std::unordered_set<std::string>>());
    } else {
      testsuite_support->get().InvalidateEverything(
          update_type, invalidate_caches["names_blocklist"]
                           .As<std::unordered_set<std::string>>({}));
    }
  }

  return formats::json::Value();
}

formats::json::Value TestsControl::ActionRunPeriodicTask(
    const formats::json::Value& request_body) const {
  const auto task_name = request_body["name"].As<std::string>();

  auto testsuite_support = testsuite_support_.Lock();

  LOG_INFO() << "Executing periodic task " << task_name << " once";
  bool status =
      testsuite_support->get().GetPeriodicTaskControl().RunPeriodicTask(
          task_name);
  if (status) {
    LOG_INFO() << "Periodic task " << task_name << " completed successfully";
  } else {
    LOG_ERROR() << "Periodic task " << task_name << " failed";
  }

  formats::json::ValueBuilder result;
  result["status"] = status;
  return result.ExtractValue();
}

formats::json::Value TestsControl::ActionSuspendPeriodicTasks(
    const formats::json::Value& request_body) const {
  auto testsuite_support = testsuite_support_.Lock();
  const auto& suspended_periodic_tasks =
      request_body["names"].As<std::vector<std::string>>();
  testsuite_support->get().GetPeriodicTaskControl().SuspendPeriodicTasks(
      suspended_periodic_tasks);
  return formats::json::Value();
}

}  // namespace server::handlers
