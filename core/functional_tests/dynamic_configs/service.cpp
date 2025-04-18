#include <string>

#include <userver/utest/using_namespace_userver.hpp>

#include <userver/components/common_component_list.hpp>
#include <userver/components/common_server_component_list.hpp>
#include <userver/components/component.hpp>
#include <userver/components/component_base.hpp>
#include <userver/dynamic_config/source.hpp>
#include <userver/dynamic_config/storage/component.hpp>
#include <userver/server/handlers/ping.hpp>
#include <userver/testsuite/testpoint.hpp>
#include <userver/testsuite/testsuite_support.hpp>
#include <userver/utils/daemon_run.hpp>

namespace functional_tests {

inline const dynamic_config::Key kTestConfig{"TEST_CONFIG_KEY", std::string("static_default")};

class DynamicConfigForwarder final : public components::ComponentBase {
public:
    static constexpr std::string_view kName = "dynamic-config-forwarder";

    DynamicConfigForwarder(const components::ComponentConfig& config, const components::ComponentContext& context)
        : components::ComponentBase(config, context),
          config_subscription_(context.FindComponent<components::DynamicConfig>().GetSource().UpdateAndListen(
              this,
              kName,
              &DynamicConfigForwarder::OnConfigUpdate
          )){};

private:
    void OnConfigUpdate(const dynamic_config::Snapshot& config) {
        TESTPOINT("config-update", [config] {
            return formats::json::ValueBuilder{config[kTestConfig]}.ExtractValue();
        }());
    }

    concurrent::AsyncEventSubscriberScope config_subscription_;
};

}  // namespace functional_tests

int main(int argc, const char* const argv[]) {
    const auto component_list = components::ComponentList()
                                    .AppendComponentList(components::CommonComponentList())
                                    .AppendComponentList(components::CommonServerComponentList())
                                    .Append<functional_tests::DynamicConfigForwarder>()
                                    .Append<server::handlers::Ping>();
    return utils::DaemonMain(argc, argv, component_list);
}
