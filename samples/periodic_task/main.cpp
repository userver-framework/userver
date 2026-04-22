#include <userver/clients/dns/component.hpp>
#include <userver/clients/http/component_list.hpp>
#include <userver/components/component_base.hpp>
#include <userver/components/component_context.hpp>
#include <userver/components/minimal_server_component_list.hpp>
#include <userver/server/handlers/http_handler_json_base.hpp>
#include <userver/server/handlers/ping.hpp>
#include <userver/server/handlers/tests_control.hpp>
#include <userver/testsuite/tasks.hpp>
#include <userver/testsuite/testsuite_support.hpp>
#include <userver/utils/daemon_run.hpp>
#include <userver/utils/periodic_task.hpp>

// Note: this is for the purposes of tests/samples only
#include <userver/utest/using_namespace_userver.hpp>

class ComponentWithPeriodicTask final : public components::ComponentBase {
public:
    static constexpr std::string_view kName = "periodic-task";

    ComponentWithPeriodicTask(const components::ComponentConfig& config, const components::ComponentContext& context)
        : components::ComponentBase(config, context)
    {
        utils::StartPeriodicTask(
            periodic_task_,
            "tick",
            utils::PeriodicTask::Settings(std::chrono::seconds(5)),
            [this] { ticks_++; },
            testsuite::GetTestsuiteTasks(context)
        );
    }

    int GetTicks() const { return ticks_; }

private:
    std::atomic<int> ticks_{0};

    // Must follow members used in the callback to respect the lifetimes
    utils::PeriodicTask periodic_task_;
};

class HandlerTicks final : public server::handlers::HttpHandlerJsonBase {
public:
    static constexpr std::string_view kName = "handler-ticks";

    HandlerTicks(const components::ComponentConfig& config, const components::ComponentContext& context)
        : server::handlers::HttpHandlerJsonBase(config, context),
          component_(context.FindComponent<ComponentWithPeriodicTask>())
    {}

    Value HandleRequestJsonThrow(const HttpRequest&, const Value&, RequestContext&) const override {
        formats::json::ValueBuilder builder = component_.GetTicks();
        return builder.ExtractValue();
    }

private:
    ComponentWithPeriodicTask& component_;
};

int main(int argc, char* argv[]) {
    auto component_list =
        components::MinimalServerComponentList()
            .Append<server::handlers::Ping>()
            .Append<HandlerTicks>()
            .Append<ComponentWithPeriodicTask>()
            .Append<clients::dns::Component>()
            .AppendComponentList(clients::http::ComponentList())
            .Append<components::TestsuiteSupport>()
            .Append<server::handlers::TestsControl>();

    return utils::DaemonMain(argc, argv, component_list);
}
