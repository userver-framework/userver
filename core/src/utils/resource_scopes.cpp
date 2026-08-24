#include <userver/utils/resource_scopes.hpp>

#include <algorithm>
#include <ranges>

#include <userver/components/component_base.hpp>
#include <userver/components/component_context.hpp>

USERVER_NAMESPACE_BEGIN

namespace utils {

void ResourceScopeStorage::DoRegister(impl::ScopePtr resource_scope, Priority priority)
{
    UINVARIANT(!scope_registration_finished_, "Scope registration is available only in component constructor");
    registered_scopes_.push_back(ScopeWithPriority{
        .priority = priority,
        .scope = std::move(resource_scope),
    });
}

void ResourceScopeStorage::SortByPriority(std::vector<ScopeWithPriority>& scopes)
{
    std::ranges::stable_sort(scopes, {}, &ScopeWithPriority::priority);
}

void ResourceScopeStorage::AfterConstruction()
{
    scope_registration_finished_ = true;
    SortByPriority(registered_scopes_);

    // A tweak to be sure in case of partial initialization only
    // already initialized scopes' before_dtr() are called
    for (auto& resource_scope : registered_scopes_) {
        resource_scope.scope->AfterConstruction();

        initialized_scopes_.push_back(std::move(resource_scope.scope));
    }
}

void ResourceScopeStorage::BeforeDestruction()
{
    SortByPriority(registered_scopes_);

    // Call Scopes' pre-destruction callbacks in reverse order
    for (auto& scope : initialized_scopes_ | std::views::reverse) {
        scope.reset();
    }
    // Factories that never ran AfterConstruction still hold captured RAII
    // handles. Drop them here so unregister runs while dependencies are
    // still alive — including constructor failure.
    for (auto& item : registered_scopes_ | std::views::reverse) {
        item.scope.reset();
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
