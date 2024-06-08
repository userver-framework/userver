#include "logflush.hpp"

#include <userver/components/component.hpp>
#include <userver/formats/json/value.hpp>
#include <userver/logging/component.hpp>
#include <userver/logging/log.hpp>

USERVER_NAMESPACE_BEGIN

namespace testsuite::impl::actions {

LogFlush::LogFlush(const components::ComponentContext& component_context)
    : logging_component_(
          component_context.FindComponent<components::Logging>()) {}

formats::json::Value LogFlush::Perform(
    const formats::json::Value& request_body) const {
  const auto logger_name =
      request_body["logger_name"].As<std::string>("default");
  auto logger = logging_component_.GetLogger(logger_name);
  logging::LogFlush(*logger);
  return {};
}

}  // namespace testsuite::impl::actions

USERVER_NAMESPACE_END
