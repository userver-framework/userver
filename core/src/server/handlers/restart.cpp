#include <userver/server/handlers/restart.hpp>

#include <userver/components/run.hpp>
#include <userver/engine/sleep.hpp>
#include <userver/utils/async.hpp>
#include <userver/utils/from_string.hpp>

USERVER_NAMESPACE_BEGIN

namespace server::handlers {

namespace {
constexpr std::string_view kDelay = "delay";
constexpr std::chrono::seconds kDelayDefault{20};
}  // namespace

Restart::Restart(const components::ComponentConfig& config, const components::ComponentContext& context)
    : HttpHandlerBase(config, context, true), health_{components::ComponentHealth::kOk} {}

std::string Restart::HandleRequestThrow(const http::HttpRequest& request, request::RequestContext&) const {
    auto delay = kDelayDefault;
    if (request.HasArg(kDelay)) {
        delay = std::chrono::seconds(utils::FromString<int>(request.GetArg(kDelay)));
    }

    health_ = components::ComponentHealth::kFatal;
    engine::DetachUnscopedUnsafe(engine::CriticalAsyncNoSpan([delay] {
        engine::InterruptibleSleepFor(std::chrono::seconds(delay));
        if (engine::current_task::ShouldCancel()) return;
        components::RequestStop();
    }));

    return "OK";
}

components::ComponentHealth Restart::GetComponentHealth() const { return health_; }

}  // namespace server::handlers

USERVER_NAMESPACE_END
