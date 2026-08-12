#pragma once

#include <userver/components/component_list.hpp>
#include <userver/server/handlers/http_handler_base.hpp>

// Note: this is for the purposes of tests/samples only
#include <userver/utest/using_namespace_userver.hpp>

namespace samples::hello {

class HelloHandler final : public server::handlers::HttpHandlerBase {
public:
    // `kName` is used as the component name in static config
    static constexpr std::string_view kName = "handler-hello-sample";

    // Component is valid after construction and is able to accept requests
    using HttpHandlerBase::HttpHandlerBase;

    std::string HandleRequestThrow(const server::http::HttpRequest& request, server::request::RequestContext&)
        const override;
};

}  // namespace samples::hello
