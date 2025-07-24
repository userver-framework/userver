#pragma once

#include <userver/components/component_list.hpp>
#include <userver/server/handlers/http_handler_base.hpp>

#include <clients/test/client.hpp>

// Note: this is for the purposes of tests/samples only
#include <userver/utest/using_namespace_userver.hpp>

namespace samples::hello {

class HelloHandler final : public server::handlers::HttpHandlerBase {
public:
    // `kName` is used as the component name in static config
    static constexpr std::string_view kName = "handler-hello-sample";

    HelloHandler(const components::ComponentConfig& config, const components::ComponentContext& component_context);

    std::string HandleRequest(server::http::HttpRequest& request, server::request::RequestContext&) const override;

private:
    ::clients::test::Client& test_;
};

}  // namespace samples::hello
