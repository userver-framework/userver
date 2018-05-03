#include "http_handler_base.hpp"

#include <logging/log.hpp>

namespace server {
namespace handlers {

HttpHandlerBase::HttpHandlerBase(
    const components::ComponentConfig& config,
    const components::ComponentContext& component_context)
    : HandlerBase(config, component_context) {}

void HttpHandlerBase::HandleRequest(const server::request::RequestBase& request,
                                    HandlerContext& context) const noexcept {
  try {
    const http::HttpRequest http_request(
        dynamic_cast<const server::http::HttpRequestImpl&>(request));
    try {
      http_request.GetHttpResponse().SetData(
          HandleRequestThrow(http_request, context));
    } catch (const std::exception& ex) {
      LOG_ERROR() << "exception in '" << HandlerName()
                  << "' handler in handle_request: " << ex.what();
      http_request.SetResponseStatus(
          server::http::HttpStatus::kInternalServerError);
      http_request.GetHttpResponse().SetData("");
    }
  } catch (const std::exception& ex) {
    LOG_ERROR() << "unable to handle request: " << ex.what();
  }
}

void HttpHandlerBase::OnRequestComplete(
    const server::request::RequestBase& request, HandlerContext& context) const
    noexcept {
  try {
    const http::HttpRequest http_request(
        dynamic_cast<const server::http::HttpRequestImpl&>(request));
    try {
      OnRequestCompleteThrow(http_request, context);
    } catch (const std::exception& ex) {
      LOG_ERROR() << "exception in '" << HandlerName()
                  << "' hander in on_request_complete: " << ex.what();
    }
  } catch (const std::exception& ex) {
    LOG_ERROR() << "unable to complete request: " << ex.what();
  }
}

}  // namespace handlers
}  // namespace server
