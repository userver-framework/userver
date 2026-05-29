#pragma once

#include <userver/engine/sleep.hpp>
#include <userver/server/handlers/http_handler_base.hpp>
#include <userver/utils/string_to_duration.hpp>

USERVER_NAMESPACE_BEGIN

namespace server::handlers::testing {

class DelayingServerHandler final : public server::handlers::HttpHandlerBase {
public:
    static constexpr std::string_view kName = "handler-delaying";

    DelayingServerHandler(const components::ComponentConfig& config, const components::ComponentContext& context)
        : HttpHandlerBase(config, context)
    {}

    std::string HandleRequestThrow(const server::http::HttpRequest& request, server::request::RequestContext&)
        const override {
        const auto& delay_arg = request.GetArg("delay");
        const auto delay = !delay_arg.empty() ? utils::StringToDuration(delay_arg) : std::chrono::milliseconds::zero();
        if (delay > std::chrono::milliseconds::zero()) {
            LOG_INFO() << "Delaying response for " << delay;
            engine::InterruptibleSleepFor(delay);
        }
        return "OK";
    }
};

}  // namespace server::handlers::testing

USERVER_NAMESPACE_END
