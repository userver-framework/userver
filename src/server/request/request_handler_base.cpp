#include "request_handler_base.hpp"

#include <stdexcept>

#include <logging/component.hpp>

#include <server/request/response_base.hpp>

namespace server {
namespace request {

RequestHandlerBase::RequestHandlerBase(
    const components::ComponentContext& component_context,
    const boost::optional<std::string>& logger_access_component,
    const boost::optional<std::string>& logger_access_tskv_component)
    : component_context_(component_context) {
  auto* logging_component =
      component_context.FindComponent<components::Logging>(
          components::Logging::kName);

  if (logger_access_component && !logger_access_component->empty()) {
    if (logging_component) {
      logger_access_ = logging_component->GetLogger(*logger_access_component);
    } else {
      throw std::runtime_error(
          "can't find logger_access component with name '" +
          *logger_access_component + '\'');
    }
  }

  if (logger_access_tskv_component && !logger_access_tskv_component->empty()) {
    if (logging_component) {
      logger_access_tskv_ =
          logging_component->GetLogger(*logger_access_tskv_component);
    } else {
      throw std::runtime_error(
          "can't find logger_access_tskv component with name '" +
          *logger_access_tskv_component + '\'');
    }
  }
}

const components::ComponentContext& RequestHandlerBase::GetComponentContext()
    const {
  return component_context_;
}

}  // namespace request
}  // namespace server
