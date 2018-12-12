#include "request_handler_base.hpp"

#include <stdexcept>

#include <logging/component.hpp>
#include <server/request/response_base.hpp>

namespace server {
namespace request {

RequestHandlerBase::RequestHandlerBase(
    const components::ComponentContext& component_context,
    const boost::optional<std::string>& logger_access_component,
    const boost::optional<std::string>& logger_access_tskv_component) {
  auto& logging_component =
      component_context.FindComponent<components::Logging>(
          components::Logging::kName);

  if (logger_access_component && !logger_access_component->empty()) {
    logger_access_ = logging_component.GetLogger(*logger_access_component);
  } else {
    LOG_INFO() << "Access log component is empty";
  }

  if (logger_access_tskv_component && !logger_access_tskv_component->empty()) {
    logger_access_tskv_ =
        logging_component.GetLogger(*logger_access_tskv_component);
  } else {
    LOG_INFO() << "Access_tskv log component is empty";
  }
}

}  // namespace request
}  // namespace server
