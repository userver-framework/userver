#include "control.hpp"

#include <userver/components/component.hpp>
#include <userver/components/statistics_storage.hpp>
#include <userver/logging/component.hpp>
#include <userver/server/handlers/exceptions.hpp>
#include <userver/testsuite/testsuite_support.hpp>
#include <userver/utils/datetime.hpp>
#include <userver/utils/mock_now.hpp>

USERVER_NAMESPACE_BEGIN

namespace testsuite::impl::actions {

Control::Control(const components::ComponentContext& component_context,
                 bool testpoint_supported)
    : testsuite_support_(
          component_context.FindComponent<components::TestsuiteSupport>()),
      logging_component_(
          component_context.FindComponent<components::Logging>()),
      testpoint_supported_(testpoint_supported) {}

formats::json::Value Control::Perform(
    const formats::json::Value& request_body) const {
  const auto testpoints = request_body["testpoints"];
  if (!testpoints.IsMissing()) {
    if (!testpoint_supported_) {
      LOG_ERROR() << "Trying to enable a testpoint, but testpoints are not "
                     "configured. Please provide 'testpoint-url' in the static "
                     "config of 'TestsControl' component.";
      throw server::handlers::ClientError();
    }
    testsuite_support_.GetTestpointControl().SetEnabledNames(
        testpoints.As<std::unordered_set<std::string>>());
  }

  auto http_allowed_urls_extra =
      request_body["http_allowed_urls_extra"]
          .As<std::optional<std::vector<std::string>>>(
              std::optional<std::vector<std::string>>{});
  if (http_allowed_urls_extra) {
    testsuite_support_.GetHttpAllowedUrlsExtra().SetAllowedUrlsExtra(
        std::move(*http_allowed_urls_extra));
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
    const auto update_type =
        cache::Parse(invalidate_caches["update_type"],
                     formats::parse::To<cache::UpdateType>());

    if (invalidate_caches.HasMember("names")) {
      testsuite_support_.GetCacheControl().InvalidateCaches(
          update_type,
          invalidate_caches["names"].As<std::unordered_set<std::string>>());
    } else {
      testsuite_support_.GetCacheControl().InvalidateAllCaches(update_type);
      testsuite_support_.GetComponentControl().InvalidateComponents();
    }
  }

  return {};
}

}  // namespace testsuite::impl::actions
USERVER_NAMESPACE_END
