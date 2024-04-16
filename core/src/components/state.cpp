#include <userver/components/state.hpp>

#include <components/component_context_impl.hpp>
#include <userver/components/component_context.hpp>

USERVER_NAMESPACE_BEGIN

namespace components {

// Hiding reference to ComponentContext from users
State::State(const ComponentContext& cc) noexcept : impl_{*cc.impl_} {}

bool State::IsAnyComponentInFatalState() const {
  return impl_.IsAnyComponentInFatalState();
}

bool State::HasDependencyOn(std::string_view component_name,
                            std::string_view dependency) const {
  return impl_.HasDependencyOn(component_name, dependency);
}

std::unordered_set<std::string_view> State::GetAllDependencies(
    std::string_view component_name) const {
  return impl_.GetAllDependencies(component_name);
}

}  // namespace components

USERVER_NAMESPACE_END
