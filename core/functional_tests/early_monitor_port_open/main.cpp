#include <userver/components/component_context.hpp>
#include <userver/components/minimal_server_component_list.hpp>
#include <userver/engine/sleep.hpp>
#include <userver/server/handlers/ping.hpp>
#include <userver/utest/using_namespace_userver.hpp>
#include <userver/utils/daemon_run.hpp>

class MonitorHandler final : public server::handlers::HttpHandlerBase {
public:
    MonitorHandler(const components::ComponentConfig& config, const components::ComponentContext& context)
        : server::handlers::HttpHandlerBase(config, context, true) {}

    static constexpr std::string_view kName = "handler-monitor";

    std::string HandleRequestThrow(const server::http::HttpRequest&, server::request::RequestContext&) const override {
        called_ = true;
        return {};
    }

    bool WasCalled() const { return called_; }

private:
    mutable std::atomic<bool> called_{false};
};

class Brake final : public components::ComponentBase {
public:
    Brake(const components::ComponentConfig& config, const components::ComponentContext& context)
        : components::ComponentBase(config, context) {
        auto& monitor_handler = context.FindComponent<MonitorHandler>();

        for (;;) {
            engine::SleepFor(std::chrono::milliseconds(100));

            if (monitor_handler.WasCalled()) {
                break;
            }
        }
    }

    static constexpr std::string_view kName = "brake";

private:
};

template <>
inline constexpr bool components::kHasValidate<Brake> = false;

template <>
inline constexpr auto components::kConfigFileMode<Brake> = ConfigFileMode::kNotRequired;

int main(int argc, char* argv[]) {
    const auto component_list =
        components::MinimalServerComponentList()
            .Append<server::handlers::Ping>()
            .Append<Brake>()
            .Append<MonitorHandler>();
    return utils::DaemonMain(argc, argv, component_list);
}
