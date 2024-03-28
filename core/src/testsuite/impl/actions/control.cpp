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

  const auto mock_now = request_body["mock_now"];
  if (!mock_now.IsMissing()) {
    const auto now = mock_now.As<std::optional<std::string>>();
    if (now) {
      // Testsuite converts timezone to UTC, then it may keep it or rip it off.
      // We interpret missing timezone as UTC by default, which is compatible
      // with this testsuite behavior.
      utils::datetime::MockNowSet(utils::datetime::Stringtime(*now));
    } else {
      utils::datetime::MockNowUnset();
    }
  }

  const auto invalidate_caches = request_body["invalidate_caches"];
  if (!invalidate_caches.IsMissing()) {
    InvalidateCaches(invalidate_caches);
  }

  return {};
}

void Control::InvalidateCaches(
    const formats::json::Value& invalidate_caches) const {
  const auto update_type =
      cache::Parse(invalidate_caches["update_type"],
                   formats::parse::To<cache::UpdateType>());
  const auto force_incremental_names =
      invalidate_caches["force_incremental_names"]
          .As<std::unordered_set<std::string>>({});

  if (invalidate_caches.HasMember("names")) {
    testsuite_support_.GetCacheControl().ResetCaches(
        update_type,
        invalidate_caches["names"].As<std::unordered_set<std::string>>(),
        force_incremental_names);
  } else {
    const auto exclude_names =
        invalidate_caches["exclude_names"].As<std::unordered_set<std::string>>(
            {});
    testsuite_support_.GetCacheControl().ResetAllCaches(
        update_type, force_incremental_names, exclude_names);
  }
}

}  // namespace testsuite::impl::actions

USERVER_NAMESPACE_END
