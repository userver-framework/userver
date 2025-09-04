#include <userver/components/state.hpp>

#include <components/component_context_impl.hpp>
#include <userver/components/component_context.hpp>
#include <userver/utils/trivial_map.hpp>

USERVER_NAMESPACE_BEGIN

namespace components {

std::string_view ToString(ServiceLifetimeStage stage) {
    static constexpr utils::TrivialBiMap kMap([](auto selector) {
        return selector()
            .Case(ServiceLifetimeStage::kLoading, "kLoading")
            .Case(ServiceLifetimeStage::kOnAllComponentsLoadedIsRunning, "kOnAllComponentsLoadedIsRunning")
            .Case(ServiceLifetimeStage::kRunning, "kRunning")
            .Case(ServiceLifetimeStage::kGracefulShutdown, "kGracefulShutdown")
            .Case(ServiceLifetimeStage::kOnAllComponentsAreStoppingIsRunning, "kOnAllComponentsAreStoppingIsRunning")
            .Case(ServiceLifetimeStage::kStopping, "kStopping");
    });

    return utils::impl::EnumToStringView(stage, kMap);
}

// Hiding reference to ComponentContext from users
State::State(const ComponentContext& cc) noexcept : impl_{cc.GetImpl(utils::impl::InternalTag{})} {}

bool State::IsAnyComponentInFatalState() const { return impl_.IsAnyComponentInFatalState(); }

ServiceLifetimeStage State::GetServiceLifetimeStage() const { return impl_.GetServiceLifetimeStage(); }

bool State::HasDependencyOn(std::string_view component_name, std::string_view dependency) const {
    return impl_.HasDependencyOn(component_name, dependency);
}

std::unordered_set<std::string_view> State::GetAllDependencies(std::string_view component_name) const {
    return impl_.GetAllDependencies(component_name);
}

}  // namespace components

USERVER_NAMESPACE_END
