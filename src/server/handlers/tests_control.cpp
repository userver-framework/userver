#include "tests_control.hpp"

#include <logging/log.hpp>
#include <utils/datetime.hpp>
#include <utils/mock_now.hpp>

#include <server/http/http_error.hpp>

namespace server {
namespace handlers {

TestsControl::TestsControl(
    const components::ComponentConfig& config,
    const components::ComponentContext& component_context)
    : HttpHandlerJsonBase(config, component_context) {}

const std::string& TestsControl::HandlerName() const {
  static const std::string kTestsControlName = kName;
  return kTestsControlName;
}

formats::json::Value TestsControl::HandleRequestJsonThrow(
    const http::HttpRequest& request, const formats::json::Value& request_body,
    request::RequestContext& context) const {
  if (request.GetMethod() != http_method::HTTP_POST) throw http::BadRequest();

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
      LOG_ERROR() << "now argument must be a string" << context.GetLogExtra();
      throw http::BadRequest();
    }
  }

  std::lock_guard<std::mutex> guard(mutex_);

  if (now)
    utils::datetime::MockNowSet(std::chrono::system_clock::from_time_t(now));
  else
    utils::datetime::MockNowUnset();

  if (invalidate_caches) {
    for (auto& invalidator : cache_invalidators_) {
      invalidator.handler();
    }
  }

  return formats::json::Value();
}

void TestsControl::RegisterCacheInvalidator(components::CacheUpdateTrait& owner,
                                            std::function<void()>&& handler) {
  std::lock_guard<std::mutex> lock(mutex_);
  cache_invalidators_.emplace_back(&owner, std::move(handler));
}

void TestsControl::UnregisterCacheInvalidator(
    components::CacheUpdateTrait& owner) {
  std::lock_guard<std::mutex> lock(mutex_);
  for (auto it = cache_invalidators_.begin(); it != cache_invalidators_.end();
       ++it) {
    if (it->owner == &owner) {
      cache_invalidators_.erase(it);
      return;
    }
  }
}

}  // namespace handlers
}  // namespace server
