#include <userver/utils/assert.hpp>

#include <userver/clients/dns/component.hpp>
#include <userver/clients/http/component.hpp>
#include <userver/clients/http/component_list.hpp>
#include <userver/components/component.hpp>
#include <userver/components/minimal_server_component_list.hpp>
#include <userver/engine/sleep.hpp>
#include <userver/http/url.hpp>
#include <userver/server/handlers/http_handler_base.hpp>
#include <userver/server/handlers/tests_control.hpp>
#include <userver/testsuite/testpoint.hpp>
#include <userver/testsuite/testsuite_support.hpp>
#include <userver/utils/daemon_run.hpp>
#include <userver/yaml_config/merge_schemas.hpp>

#include <userver/utest/using_namespace_userver.hpp>

class SlowComponent final : public components::ComponentBase {
public:
    static constexpr std::string_view kName = "slow-component";

    SlowComponent(const components::ComponentConfig& config, const components::ComponentContext& context)
        : components::ComponentBase(config, context) {
        context.FindComponent<server::handlers::TestsControl>();
        context.FindComponent<components::TestsuiteSupport>().GetTestpointControl().SetAllEnabled();
    }

    void OnAllComponentsLoaded() override {
        TESTPOINT("slow-component-loading", {});
        // Testsuite test should finish until 120s end
        engine::InterruptibleSleepFor(std::chrono::seconds(120));

        if (!engine::current_task::ShouldCancel()) {
            throw std::runtime_error("Seems like testsuite test is still running, a bug in test?");
        }
    }
};

int main(int argc, char* argv[]) {
    const auto component_list =
        components::MinimalServerComponentList()
            .AppendComponentList(clients::http::ComponentList())
            .Append<clients::dns::Component>()
            .Append<components::TestsuiteSupport>()
            .Append<server::handlers::TestsControl>()
            .Append<SlowComponent>();
    return utils::DaemonMain(argc, argv, component_list);
}
