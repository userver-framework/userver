#include <server/handlers/tests_control.hpp>

#include <cache/cache_invalidator.hpp>
#include <logging/log.hpp>
#include <server/http/http_error.hpp>
#include <utils/datetime.hpp>
#include <utils/mock_now.hpp>

namespace server {
namespace handlers {

TestsControl::TestsControl(
    const components::ComponentConfig& config,
    const components::ComponentContext& component_context)
    : HttpHandlerJsonBase(config, component_context),
      cache_invalidator_(
          component_context.FindComponent<components::CacheInvalidator>()) {}

const std::string& TestsControl::HandlerName() const {
  static const std::string kTestsControlName = kName;
  return kTestsControlName;
}

formats::json::Value TestsControl::HandleRequestJsonThrow(
    const http::HttpRequest& request, const formats::json::Value& request_body,
    request::RequestContext&) const {
  if (request.GetMethod() != http::HttpMethod::kPost) throw http::BadRequest();

  bool invalidate_caches = false;
  const formats::json::Value& value = request_body["invalidate_caches"];
  if (value.isBool()) {
    invalidate_caches = value.asBool();
  }

  std::time_t now = 0;
  if (request_body.HasMember("now")) {
    const formats::json::Value& value = request_body["now"];
    if (value.isString()) {
      now =
          std::chrono::duration_cast<std::chrono::seconds>(
              utils::datetime::Stringtime(value.asString()).time_since_epoch())
              .count();
    } else {
      LOG_ERROR() << "'now' argument must be a string";
      throw http::BadRequest();
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
