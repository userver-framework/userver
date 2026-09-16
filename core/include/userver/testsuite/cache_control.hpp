#pragma once

/// @file userver/testsuite/cache_control.hpp
/// @brief @copybrief testsuite::CacheControl

#include <functional>
#include <memory>
#include <string>
#include <unordered_set>

#include <userver/cache/update_type.hpp>
#include <userver/components/component_fwd.hpp>
#include <userver/utils/assert.hpp>
#include <userver/utils/impl/internal_tag.hpp>

USERVER_NAMESPACE_BEGIN

namespace cache {
class CacheUpdateTrait;
struct Config;
}  // namespace cache

namespace components {
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
/// @snippet core/src/testsuite/cache_control_test.cpp  sample
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
    /// Every registered resetter whose name is in @a reset_only_names is
    /// invoked. Several resetters may share a name: for example a periodic
    /// @ref cache::CacheUpdateTrait resetter and a later custom
    /// @ref RegisterCacheResetter.
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

    // For internal use only. Use testsuite::RegisterCacheResetter instead
    CacheResetRegistration RegisterCache(
        utils::impl::InternalTag,
        std::string_view name,
        std::function<void(cache::UpdateType)> reset
    );

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
        const std::unordered_set<std::string>* reset_only_names,
        const std::unordered_set<std::string>& force_incremental_names,
        const std::unordered_set<std::string>* exclude_names
    );

    void DoResetCachesConcurrently(
        cache::UpdateType update_type,
        const std::unordered_set<std::string>* reset_only_names,
        std::unordered_set<std::string>& names_left_to_encounter,
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
/// Prefer @ref RegisterCacheResetter so that the resetter is registered after
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
/// @see testsuite::RegisterCacheResetter
CacheControl& FindCacheControl(const components::ComponentContext& context);

namespace impl {

void DoRegisterCacheScope(const components::ComponentContext& context, std::function<void(cache::UpdateType)> reset);

template <typename Component>
std::function<void(cache::UpdateType)> BindCacheResetter(Component* self, void (Component::*reset_method)()) {
    UASSERT(self);
    UASSERT(reset_method);
    return [self, reset_method]([[maybe_unused]] cache::UpdateType update_type) { (self->*reset_method)(); };
}

template <typename Component>
std::function<void(cache::UpdateType)> BindCacheResetter(
    Component* self,
    void (Component::*reset_method)(cache::UpdateType)
) {
    UASSERT(self);
    UASSERT(reset_method);
    return [self, reset_method](cache::UpdateType update_type) { (self->*reset_method)(update_type); };
}

}  // namespace impl

/// @brief Registers a cache resetter bound to the component lifetime.
///
/// The resetter is registered after the component constructor finishes
/// and is unregistered just before the destructor runs.
///
/// Several cache resetters for the same component are invoked sequentially
/// in registration order.
///
/// Typical usage:
/// @code
/// testsuite::RegisterCacheResetter(context, this, &MyCache::ResetCache);
/// @endcode
///
/// @warning The function should be called in the component's constructor
/// *after* all FindComponent calls. This ensures that reset will first be
/// called for dependencies, then for dependent components.
template <typename Component>
void RegisterCacheResetter(
    const components::ComponentContext& context,
    Component* self,
    void (Component::*reset_method)()
) {
    impl::DoRegisterCacheScope(context, impl::BindCacheResetter(self, reset_method));
}

/// @overload The resetter additionally receives the requested
/// @ref cache::UpdateType.
///
/// Use this when the hook must distinguish a full invalidation from an
/// incremental one. Typical cases:
/// - a cache that supports both update types but is not periodic, so it is
///   not a @ref components::CachingComponentBase: values are pushed in,
///   for example by a handler called from a sidecar;
/// - an extra resetter on a @ref components::CachingComponentBase cache that
///   must know the requested update type to adjust incoming data for testsuite.
template <typename Component>
void RegisterCacheResetter(
    const components::ComponentContext& context,
    Component* self,
    void (Component::*reset_method)(cache::UpdateType)
) {
    impl::DoRegisterCacheScope(context, impl::BindCacheResetter(self, reset_method));
}

/// @deprecated Use @ref RegisterCacheResetter instead.
///
/// Same as @ref RegisterCacheResetter for a `void()` hook. The testsuite
/// `update_type` is ignored.
template <typename Component>
void RegisterCacheScope(
    const components::ComponentContext& context,
    Component* self,
    void (Component::*reset_method)()
) {
    RegisterCacheResetter(context, self, reset_method);
}

/// @deprecated Use @ref RegisterCacheResetter instead.
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
    return cc.RegisterCache(
        utils::impl::InternalTag{},
        components::GetCurrentComponentName(context),
        impl::BindCacheResetter(self, reset_method)
    );
}

}  // namespace testsuite

USERVER_NAMESPACE_END
