#include <userver/utils/resource_scopes.hpp>

#include <ranges>

#include <userver/components/component_base.hpp>
#include <userver/components/component_context.hpp>

USERVER_NAMESPACE_BEGIN

namespace utils {

void ResourceScopeStorage::DoRegister(impl::ScopePtr resource_scope)
{
    UINVARIANT(!scope_registration_finished_, "Scope registration is available only in component constructor");
    registered_scopes_.push_back(std::move(resource_scope));
}

void ResourceScopeStorage::AfterConstruction()
{
    scope_registration_finished_ = true;

    // A tweak to be sure in case of partial initialization only
    // already initialized scopes' before_dtr() are called
    for (auto& resource_scope : registered_scopes_) {
        resource_scope->AfterConstruction();

        initialized_scopes_.push_back(std::move(resource_scope));
    }
}

void ResourceScopeStorage::BeforeDestruction()
{
    // Call Scopes' pre-destruction callbacks in reverse order
    for (auto& scope : initialized_scopes_ | std::views::reverse) {
        scope.reset();
    }
}

ResourceScopeStorage& LocateDependency(
    components::WithType<ResourceScopeStorage>,
    const components::ComponentConfig&,
    const components::ComponentContext& context
)
{
    return context.Scopes();
}

}  // namespace utils

USERVER_NAMESPACE_END
