#pragma once

/// @file server/handlers/http_handler_json_base.hpp

#include <server/handlers/http_handler_base.hpp>

namespace server {
namespace handlers {

/// Convenient base for handlers that accept requests with body in
/// json format.
class HttpHandlerJsonBase : public HttpHandlerBase {
 public:
  HttpHandlerJsonBase(const components::ComponentConfig& config,
                      const components::ComponentContext& component_context);

  std::string HandleRequestThrow(const http::HttpRequest& request,
                                 request::RequestContext& context) const final;

  virtual formats::json::Value HandleRequestJsonThrow(
      const http::HttpRequest& request,
      const formats::json::Value& request_json,
      request::RequestContext& context) const = 0;

  /// @returns A pointer to json request if it was parsed successfully or
  /// nullptr otherwise.
  const formats::json::Value* GetRequestJson(
      const request::RequestContext& context) const;

  /// @returns a pointer to json response if it was returned successfully by
  /// `HandleRequestJsonThrow()` or nullptr otherwise.
  const formats::json::Value* GetResponseJson(
      const request::RequestContext& context) const;

 private:
  void ParseRequestData(const http::HttpRequest& request,
                        request::RequestContext& context) const final;
};

}  // namespace handlers
}  // namespace server
