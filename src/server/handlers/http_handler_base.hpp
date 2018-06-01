#pragma once

#include <string>

#include <server/http/http_request.hpp>
#include <server/http/http_response.hpp>
#include <server/request/need_log_request_checker_base_component.hpp>
#include <server/request/request_base.hpp>

#include "handler_base.hpp"

namespace server {
namespace handlers {

class HttpHandlerBase : public HandlerBase {
 public:
  HttpHandlerBase(const components::ComponentConfig& config,
                  const components::ComponentContext& component_context);

  virtual void HandleRequest(const request::RequestBase& request,
                             request::RequestContext& context) const
      noexcept override;
  virtual void OnRequestComplete(const request::RequestBase& request,
                                 request::RequestContext& context) const
      noexcept override;

  virtual const std::string& HandlerName() const = 0;
  virtual std::string HandleRequestThrow(
      const http::HttpRequest& request,
      request::RequestContext& context) const = 0;
  virtual void OnRequestCompleteThrow(
      const http::HttpRequest& /*request*/,
      request::RequestContext& /*context*/) const {}

 private:
  const components::NeedLogRequestCheckerBase* need_log_request_checker_;
};

}  // namespace handlers
}  // namespace server
