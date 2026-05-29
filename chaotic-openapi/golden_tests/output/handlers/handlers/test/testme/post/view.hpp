#pragma once

#include <handlers/test/testme/post/requests.hpp>
#include <handlers/test/testme/post/responses.hpp>
#include <string>
#include <userver/chaotic/openapi/server/dependencies.hpp>
#include <userver/formats/json/value.hpp>
#include <userver/server/http/http_request.hpp>
#include <userver/server/request/request_context.hpp>

namespace handlers::test::testme::post {

struct HandlerTag;

class View final {
public:
    using Deps = USERVER_NAMESPACE::chaotic::openapi::server::dependencies::ForHandler<HandlerTag>;

    static Response Handle(Request&& request, Deps&& deps);

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
        USERVER_NAMESPACE::server::request::RequestContext& context);
    */
};

}  // namespace handlers::test::testme::post
