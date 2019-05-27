#include <server/handlers/tests_control.hpp>

#include <cache/cache_invalidator.hpp>
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

  bool invalidate_caches = false;
  const formats::json::Value& value = request_body["invalidate_caches"];
  if (value.IsBool()) {
    invalidate_caches = value.As<bool>();
  }

  std::time_t now = 0;
  if (request_body.HasMember("now")) {
    const formats::json::Value& value = request_body["now"];
    if (value.IsString()) {
      now = std::chrono::duration_cast<std::chrono::seconds>(
                utils::datetime::Stringtime(value.As<std::string>())
                    .time_since_epoch())
                .count();
    } else {
      LOG_ERROR() << "'now' argument must be a string";
      throw ClientError();
    }
  }

  std::lock_guard<engine::Mutex> guard(mutex_);

  if (now)
    utils::datetime::MockNowSet(std::chrono::system_clock::from_time_t(now));
  else
    utils::datetime::MockNowUnset();

  if (invalidate_caches) cache_invalidator_.InvalidateCaches();

  return formats::json::Value();
}

}  // namespace handlers
}  // namespace server
