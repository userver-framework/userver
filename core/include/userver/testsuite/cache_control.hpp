#pragma once

/// @file userver/testsuite/cache_control.hpp
/// @brief @copybrief testsuite::CacheControl

#include <functional>
#include <memory>
#include <string>
#include <type_traits>
#include <unordered_set>

#include <userver/cache/update_type.hpp>
#include <userver/components/component_fwd.hpp>
#include <userver/utils/assert.hpp>
#include <userver/utils/move_only_function.hpp>

USERVER_NAMESPACE_BEGIN

namespace cache {
class CacheUpdateTrait;
struct Config;
}  // namespace cache

namespace components {
class RawComponentBase;
class State;
}  // namespace components

namespace testsuite {

namespace impl {
enum class PeriodicUpdatesMode { kDefault, kEnabled, kDisabled };

using CacheReverseDependencies = std::unordered_set<std::string>;

CacheReverseDependencies GetDefaultCacheReverseDependencies();

}  // namespace impl

class CacheResetRegistration;

/// @brief Testsuite interface for caches and cache-like components.
///
/// If a component stores transient state that may be carried between tests,
/// or stores caches that may become stale, then it should register its resetter
/// here. Example:
///
/// @snippet testsuite/cache_control_test.cpp  sample
///
/// Testsuite will then call this hook in the beginning of each test.
/// You can also reset a specific cache in testsuite explicitly as follows:
///
/// @code
/// service_client.invalidate_caches(names=['your-cache-name'])
/// @endcode
///
/// CacheControl is normally acquired through testsuite::FindCacheControl.
///
/// All methods are coro-safe.
class CacheControl final {
public:
    /// @brief Reset all the registered caches.
    ///
    /// @a update_type is used by caches derived from
    /// @a component::CachingComponentBase.
    void ResetAllCaches(
        cache::UpdateType update_type,
        const std::unordered_set<std::string>& force_incremental_names,
        const std::unordered_set<std::string>& exclude_names
    );

    /// @brief Reset caches with the specified @a names.
    ///
    /// @a update_type is used by caches derived from
    /// @a component::CachingComponentBase.
    void ResetCaches(
        cache::UpdateType update_type,
        std::unordered_set<std::string> reset_only_names,
        const std::unordered_set<std::string>& force_incremental_names
    );

    CacheControl(CacheControl&&) = delete;
    CacheControl& operator=(CacheControl&&) = delete;

    /// @cond
    // For internal use only.
    struct UnitTests {
        explicit UnitTests() = default;
    };

    enum class ExecPolicy {
        kSequential,
        kConcurrent,
    };

    CacheControl(impl::PeriodicUpdatesMode, UnitTests);
    CacheControl(
        impl::PeriodicUpdatesMode,
        ExecPolicy,
        components::State,
        impl::CacheReverseDependencies reverse_dependencies
    );
    ~CacheControl();

    // For internal use only.
    bool IsPeriodicUpdateEnabled(const cache::Config& cache_config, const std::string& cache_name) const;

    // For internal use only.
    CacheResetRegistration RegisterPeriodicCache(cache::CacheUpdateTrait& cache);

    // For internal use only. Use testsuite::RegisterCacheScoped instead
    template <typename Component>
    CacheResetRegistration RegisterCache(Component* self, std::string_view name, void (Component::*reset_method)());

    struct CacheInfo final {
        std::string name;
        std::function<void(cache::UpdateType)> reset;
        bool needs_span{true};
    };
    struct CacheInfoNode;
    using CacheInfoIterator = CacheInfoNode*;

    // For internal use only.
    CacheInfoIterator DoRegisterCache(CacheInfo&& info);
    /// @endcond
private:
    friend class CacheResetRegistration;

    class CacheResetJob;

    void DoResetCaches(
        cache::UpdateType update_type,
        std::unordered_set<std::string>* reset_only_names,
        const std::unordered_set<std::string>& force_incremental_names,
        const std::unordered_set<std::string>* exclude_names
    );

    void DoResetCachesConcurrently(
        cache::UpdateType update_type,
        std::unordered_set<std::string>* reset_only_names,
        const std::unordered_set<std::string>& force_incremental_names,
        const std::unordered_set<std::string>* exclude_names
    );

    void UnregisterCache(CacheInfoIterator) noexcept;

    static void DoResetSingleCache(
        const CacheInfo& info,
        cache::UpdateType update_type,
        const std::unordered_set<std::string>& force_incremental_names
    );

    struct Impl;
    std::unique_ptr<Impl> impl_;
};

/// @brief RAII helper for testsuite registration.
///
/// Removes the associated resetter automatically on destruction.
///
/// Prefer @ref RegisterCacheScoped so that the resetter is registered after
/// the component constructor and unregistered just before the destructor.
/// Otherwise store the registration as a member after the rest of
/// the component's fields.
/// @see testsuite::CacheControl
class [[nodiscard]] CacheResetRegistration final {
public:
    CacheResetRegistration() noexcept;

    CacheResetRegistration(CacheResetRegistration&&) noexcept;
    CacheResetRegistration& operator=(CacheResetRegistration&&) noexcept;
    ~CacheResetRegistration();

    /// Unregister the cache component explicitly.
    /// `Unregister` is called in the destructor automatically.
    void Unregister() noexcept;

    /// @cond
    // For internal use only.
    CacheResetRegistration(CacheControl&, CacheControl::CacheInfoIterator);
    /// @endcond

private:
    CacheControl* cache_control_{nullptr};
    CacheControl::CacheInfoIterator cache_info_iterator_{};
};

/// The method for acquiring testsuite::CacheControl in the component system.
///
/// @see testsuite::RegisterCacheScoped
CacheControl& FindCacheControl(const components::ComponentContext& context);

namespace impl {

void DoRegisterCacheScoped(
    const components::ComponentContext& context,
    utils::move_only_function<CacheResetRegistration()> factory
);

}  // namespace impl

/// @brief Registers a cache resetter bound to the component lifetime.
///
/// The resetter is registered after the component constructor finishes
/// and is unregistered just before the destructor runs.
///
/// Typical usage:
/// @code
/// testsuite::RegisterCacheScoped(context, this, &MyCache::ResetCache);
/// @endcode
///
/// @warning The function should be called in the component's constructor
/// *after* all FindComponent calls. This ensures that reset will first be
/// called for dependencies, then for dependent components.
template <typename Component>
void RegisterCacheScoped(
    const components::ComponentContext& context,
    Component* self,
    void (Component::*reset_method)()
) {
    auto& cc = testsuite::FindCacheControl(context);
    auto name = std::string{components::GetCurrentComponentName(context)};
    impl::DoRegisterCacheScoped(context, [&cc, self, name = std::move(name), reset_method] {
        return cc.RegisterCache(self, name, reset_method);
    });
}

/// @deprecated Use @ref RegisterCacheScoped instead.
/// The returned handle must be kept alive to keep supporting cache resetting.
///
/// @warning The function should be called in the component's constructor
/// *after* all FindComponent calls. This ensures that reset will first be
/// called for dependencies, then for dependent components.
template <typename Component>
CacheResetRegistration RegisterCache(
    const components::ComponentContext& context,
    Component* self,
    void (Component::*reset_method)()
) {
    auto& cc = testsuite::FindCacheControl(context);
    return cc.RegisterCache(self, components::GetCurrentComponentName(context), reset_method);
}

/// @cond
template <typename Component>
CacheResetRegistration CacheControl::RegisterCache(
    Component* self,
    std::string_view name,
    void (Component::*reset_method)()
) {
    static_assert(
        std::is_base_of_v<components::RawComponentBase, Component>,
        "CacheControl can only be used with components"
    );
    UASSERT(self);
    UASSERT(reset_method);

    CacheInfo info;
    info.name = std::string{name};
    info.reset = [self, reset_method]([[maybe_unused]] cache::UpdateType) { (self->*reset_method)(); };
    info.needs_span = true;

    auto iter = DoRegisterCache(std::move(info));
    return CacheResetRegistration(*this, std::move(iter));
}
/// @endcond

}  // namespace testsuite

USERVER_NAMESPACE_END
