#include <userver/utest/using_namespace_userver.hpp>

#include "hello_service.hpp"

#include <userver/server/handlers/http_handler_base.hpp>

#include "say_hello.hpp"

namespace samples::hello {

namespace {

class Hello final : public server::handlers::HttpHandlerBase {
public:
    // `kName` is used as the component name in static config
    static constexpr std::string_view kName = "handler-hello-sample";

    // Component is valid after construction and is able to accept requests
    using HttpHandlerBase::HttpHandlerBase;

    /// [Handler]
    std::string HandleRequestThrow(const server::http::HttpRequest& request, server::request::RequestContext&)
        const override {
        request.GetHttpResponse().SetContentType(http::content_type::kApplicationJson);

        auto request_json = formats::json::FromString(request.RequestBody());

        // Use generated parser for As()
        auto request_dom = request_json.As<HelloRequestBody>();

        // request_dom and response_dom have generated types
        auto response_dom = SayHelloTo(request_dom);

        // Use generated serializer for ValueBuilder()
        auto response_json = formats::json::ValueBuilder{response_dom}.ExtractValue();
        return formats::json::ToString(response_json);
    }
    /// [Handler]
};

}  // namespace

void AppendHello(components::ComponentList& component_list) { component_list.Append<Hello>(); }

}  // namespace samples::hello
