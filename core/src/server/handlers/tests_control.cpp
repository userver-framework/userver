#include <server/handlers/tests_control.hpp>

#include <cache/update_type.hpp>
#include <clients/http/component.hpp>
#include <logging/log.hpp>
#include <server/http/http_error.hpp>
#include <testsuite/testpoint.hpp>
#include <testsuite/testsuite_support.hpp>
#include <utils/datetime.hpp>
#include <utils/mock_now.hpp>

namespace server {
namespace handlers {

TestsControl::TestsControl(
    const components::ComponentConfig& config,
    const components::ComponentContext& component_context)
    : HttpHandlerJsonBase(config, component_context),
      testsuite_support_(
          component_context.FindComponent<components::TestsuiteSupport>()) {
  auto testpoint_url = config.ParseOptionalString("testpoint-url");
  if (testpoint_url) {
    auto& http_client =
        component_context.FindComponent<components::HttpClient>()
            .GetHttpClient();
    auto testpoint_timeout =
        config.ParseDuration("testpoint-timeout", std::chrono::seconds(1));
    auto& tp = testsuite::impl::TestPoint::GetInstance();
    tp.Setup(http_client, *testpoint_url, testpoint_timeout);
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

  bool invalidate_caches = false;
  const auto& invalidate_caches_value = request_body["invalidate_caches"];
  if (invalidate_caches_value.IsBool()) {
    invalidate_caches = invalidate_caches_value.As<bool>();
  }

  const auto& clean_update = request_body["cache_clean_update"];
  cache::UpdateType update_type = clean_update.As<bool>(true)
                                      ? cache::UpdateType::kFull
                                      : cache::UpdateType::kIncremental;

  boost::optional<std::chrono::system_clock::time_point> now;
  if (request_body.HasMember("now")) {
    const formats::json::Value& value = request_body["now"];
    if (value.IsString()) {
      now = utils::datetime::Stringtime(value.As<std::string>());
    } else {
      LOG_ERROR() << "'now' argument must be a string";
      throw ClientError();
    }
  }

  auto testsuite_support = testsuite_support_.Lock();

  if (now)
    utils::datetime::MockNowSet(*now);
  else
    utils::datetime::MockNowUnset();

  if (invalidate_caches) {
    auto names = request_body["names"];
    if (names.IsMissing()) {
      testsuite_support->get().InvalidateEverything(update_type);
    } else {
      testsuite_support->get().InvalidateCaches(
          update_type, names.As<std::vector<std::string>>());
    }
  }

  return formats::json::Value();
}

formats::json::Value TestsControl::ActionRunPeriodicTask(
    const formats::json::Value& request_body) const {
  const auto task_name = request_body["name"].As<std::string>();

  auto testsuite_support = testsuite_support_.Lock();

  LOG_INFO() << "Running periodic task " << task_name;
  bool status =
      testsuite_support->get().GetPeriodicTaskControl().RunPeriodicTask(
          task_name);
  LOG_INFO() << "Periodic task " << task_name << " finished with status "
             << status;

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

}  // namespace handlers
}  // namespace server
