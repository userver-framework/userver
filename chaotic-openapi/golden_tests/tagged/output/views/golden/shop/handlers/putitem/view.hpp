#pragma once

#include <string>

#include <userver/chaotic/openapi/server/dependencies.hpp>
#include <userver/server/http/http_request.hpp>
#include <userver/server/request/request_context.hpp>
#include <userver/formats/json/value.hpp>
#include <golden/shop/handlers/putitem/requests.hpp>
#include <golden/shop/handlers/putitem/responses.hpp>

namespace golden::shop::handlers::putitem {

struct HandlerTag;

class View final {
public:
using Deps = USERVER_NAMESPACE::chaotic::openapi::server::dependencies::ForHandler<HandlerTag>;
using RequestContext = USERVER_NAMESPACE::server::request::RequestContext;

static Response Handle(Request&& request, Deps&& deps, RequestContext& context);

/* Uncomment, if you want to define a custom logging for request/response body.
* E.g. you want to log several fields, but omit the others (secrets, etc.).
*
static std::string GetRequestBodyForLogging(
const USERVER_NAMESPACE::formats::json::Value& body);

// Logger for 'invalid JSON body' request
static std::string GetInvalidRequestBodyForLogging(
const USERVER_NAMESPACE::server::http::HttpRequest& request);

static std::string GetResponseForLogging(
const Response& response,
const std::string& serialized_response,
RequestContext& context);
*/
};

}  // namespace golden::shop::handlers::putitem
