#include <userver/utils/resource_scopes.hpp>

#include <algorithm>
#include <ranges>

#include <userver/components/component_base.hpp>
#include <userver/components/component_context.hpp>
#include <userver/utils/assert.hpp>

USERVER_NAMESPACE_BEGIN

namespace utils {

void ResourceScopeStorage::DoRegister(impl::ScopePtr resource_scope, Priority priority) {
    switch (state_) {
        case State::kConstruction:
            registered_scopes_.push_back(ScopeWithPriority{
                .priority = priority,
                .scope = std::move(resource_scope),
            });
            return;
        case State::kAfterConstruction:
        case State::kReady:
            OpenAndKeep(std::move(resource_scope));
            return;
        case State::kDestruction:
            UASSERT_MSG(false, "Registering a resource scope on a partially destroyed object is unsafe");
            [[fallthrough]];
        case State::kBeforeDestruction:
            OpenAndClose(std::move(resource_scope));
            return;
    }
}

void ResourceScopeStorage::OpenAndKeep(impl::ScopePtr resource_scope) {
    resource_scope->AfterConstruction();
    initialized_scopes_.push_back(std::move(resource_scope));
}

void ResourceScopeStorage::OpenAndClose(impl::ScopePtr resource_scope) { resource_scope->AfterConstruction(); }

void ResourceScopeStorage::SortByPriority(std::vector<ScopeWithPriority>& scopes) noexcept {
    std::ranges::stable_sort(scopes, {}, &ScopeWithPriority::priority);
}

void ResourceScopeStorage::AfterConstruction() {
    UINVARIANT(state_ == State::kConstruction, "AfterConstruction must be called once after construction");
    state_ = State::kAfterConstruction;
    SortByPriority(registered_scopes_);

    try {
        // A tweak to be sure in case of partial initialization only
        // already initialized scopes' before_dtr() are called
        for (auto& resource_scope : registered_scopes_) {
            OpenAndKeep(std::move(resource_scope.scope));
        }
    } catch (...) {
        BeforeDestruction();
        throw;
    }

    registered_scopes_.clear();
    state_ = State::kReady;
}

void ResourceScopeStorage::BeforeDestruction() noexcept {
    if (state_ == State::kBeforeDestruction || state_ == State::kDestruction) {
        return;
    }
    state_ = State::kBeforeDestruction;
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
    state_ = State::kDestruction;
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
