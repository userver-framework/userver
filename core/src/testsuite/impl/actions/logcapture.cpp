#include "logcapture.hpp"

#include <userver/components/component.hpp>
#include <userver/formats/json/value.hpp>
#include <userver/logging/component.hpp>

USERVER_NAMESPACE_BEGIN

namespace testsuite::impl::actions {

LogCapture::LogCapture(const components::ComponentContext& component_context)
    : logging_component_(
          component_context.FindComponent<components::Logging>()) {}

formats::json::Value LogCapture::Perform(
    const formats::json::Value& request_body) const {
  const auto socket_logging = request_body["socket_logging_duplication"];
  const auto log_level = logging::OptionalLevelFromString(
      request_body["log_level"].As<std::optional<std::string>>());
  const auto enable = socket_logging.As<bool>();
  if (enable) {
    logging_component_.StartSocketLoggingDebug(log_level);
  } else {
    logging_component_.StopSocketLoggingDebug(log_level);
  }

  return {};
}

}  // namespace testsuite::impl::actions

USERVER_NAMESPACE_END
