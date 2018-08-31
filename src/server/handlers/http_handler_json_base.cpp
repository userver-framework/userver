#include "http_handler_json_base.hpp"

#include <json/reader.h>
#include <json/writer.h>

#include <server/http/http_error.hpp>

namespace server {
namespace handlers {

HttpHandlerJsonBase::HttpHandlerJsonBase(
    const components::ComponentConfig& config,
    const components::ComponentContext& component_context)
    : HttpHandlerBase(config, component_context) {}

std::string HttpHandlerJsonBase::HandleRequestThrow(
    const http::HttpRequest& request, request::RequestContext& context) const {
  Json::Reader reader;
  Json::Value request_json;

  if (!reader.parse(request.RequestBody(), request_json)) {
    throw http::BadRequest(
        "Invalid JSON body",
        "Invalid JSON body: " + reader.getFormattedErrorMessages());
  }

  Json::Value response_json =
      HandleRequestJsonThrow(request, request_json, context);
  Json::FastWriter writer;
  return writer.write(response_json);
}

}  // namespace handlers
}  // namespace server
