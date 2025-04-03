#pragma once

#include <signal.h>
#include <unistd.h>

#include <userver/server/handlers/http_handler_base.hpp>
#include <userver/utest/using_namespace_userver.hpp>

namespace handlers {

class Sigterm final : public server::handlers::HttpHandlerBase {
public:
    static constexpr std::string_view kName = "handler-sigterm";

    Sigterm(const components::ComponentConfig& config, const components::ComponentContext& context)
        : HttpHandlerBase(config, context) {}

    std::string HandleRequestThrow(const server::http::HttpRequest& /*request*/, server::request::RequestContext&)
        const override {
        kill(getpid(), SIGTERM);
        return {};
    }
};

}  // namespace handlers
