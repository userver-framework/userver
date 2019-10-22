#include <server/handlers/tests_control.hpp>

#include <cache/cache_invalidator.hpp>
#include <cache/update_type.hpp>
#include <clients/http/component.hpp>
#include <logging/log.hpp>
#include <server/http/http_error.hpp>
#include <utils/datetime.hpp>
#include <utils/mock_now.hpp>
#include <utils/testpoint.hpp>

namespace server {
namespace handlers {

TestsControl::TestsControl(
    const components::ComponentConfig& config,
    const components::ComponentContext& component_context)
    : HttpHandlerJsonBase(config, component_context),
      cache_invalidator_(
          component_context.FindComponent<components::CacheInvalidator>()) {
  auto testpoint_url = config.ParseOptionalString("testpoint-url");
  if (testpoint_url) {
    auto& http_client =
        component_context.FindComponent<components::HttpClient>()
            .GetHttpClient();
    auto testpoint_timeout =
        config.ParseDuration("testpoint-timeout", std::chrono::seconds(1));
    auto& tp = utils::TestPoint::GetInstance();
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

  auto cache_invalidator = cache_invalidator_.Lock();

  if (now)
    utils::datetime::MockNowSet(*now);
  else
    utils::datetime::MockNowUnset();

  if (invalidate_caches) {
    cache_invalidator->get().InvalidateCaches(update_type);
  }

  return formats::json::Value();
}

formats::json::Value TestsControl::ActionRunPeriodicTask(
    const formats::json::Value& request_body) const {
  const auto task_name = request_body["name"].As<std::string>();

  auto cache_invalidator = cache_invalidator_.Lock();

  LOG_INFO() << "Running periodic task " << task_name;
  bool status = cache_invalidator->get().RunPeriodicTask(task_name);
  LOG_INFO() << "Periodic task " << task_name << " finished with status "
             << status;

  formats::json::ValueBuilder result;
  result["status"] = status;
  return result.ExtractValue();
}

}  // namespace handlers
}  // namespace server
