#pragma once

/// @file userver/utils/resource_scopes.hpp
/// @brief @copybrief utils::ResourceScopeStorage

#include <concepts>
#include <cstdint>
#include <memory>
#include <optional>
#include <type_traits>
#include <utility>
#include <vector>

#include <userver/compiler/impl/lifetime.hpp>
#include <userver/components/component_fwd.hpp>
#include <userver/utils/assert.hpp>
#include <userver/utils/impl/internal_tag.hpp>
#include <userver/utils/move_only_function.hpp>

USERVER_NAMESPACE_BEGIN

namespace utils {

namespace impl {
class ScopeBase {
public:
    virtual ~ScopeBase() = default;

    virtual void AfterConstruction() = 0;
};

template <typename Handle>
class Scope final : public ScopeBase {
public:
    using AfterConstructionCallback = utils::move_only_function<Handle()>;

    explicit Scope(AfterConstructionCallback after_construction)
        : after_construction_(std::move(after_construction))
    {}

    void AfterConstruction() override { before_destruction_.emplace(after_construction_()); }

private:
    AfterConstructionCallback after_construction_;
    std::optional<Handle> before_destruction_;
};

template <>
class Scope<void> final : public ScopeBase {
public:
    using AfterConstructionCallback = utils::move_only_function<void()>;

    explicit Scope(AfterConstructionCallback after_construction)
        : after_construction_(std::move(after_construction))
    {}

    void AfterConstruction() override { after_construction_(); }

private:
    AfterConstructionCallback after_construction_;
};

/// @brief An object of ScopePtr defines actions to do after
/// a component is constructed and just before it is destroyed.
///
/// @see @ref components::ComponentContext::Scopes
using ScopePtr = std::unique_ptr<impl::ScopeBase>;

}  // namespace impl

/// @brief Explicit opt-in to managing resource-scope lifetime manually.
/// Prefer @ref utils::WithResourceScopes, @ref utils::MakeWithResourceScopes or
/// @ref utils::MakeUniqueWithResourceScopes instead.
struct UnsafeManualResourceScopes final {
    constexpr explicit UnsafeManualResourceScopes() noexcept = default;
};

/// @brief Runs resource setup after object construction and cleanup before object destruction.
///
/// During construction, use @ref utils::ResourceScopeStorage::Register to queue setup functions. The component system
/// or scope wrapper calls @ref utils::ResourceScopeStorage::AfterConstruction after all constructors, including derived
/// constructors, finish. It calls @ref utils::ResourceScopeStorage::BeforeDestruction to destroy the returned RAII
/// handles before the object's destructor starts.
///
/// See @ref resource_scopes_parameter "choosing scopes" for ownership and wrapper selection, and
/// @ref resource_scopes_register "using Register directly" for custom resources.
///
/// @snippet core/src/components/resource_scopes_test.cpp ResourceScopeStorage - HappyPathOrder
class ResourceScopeStorage final {
public:
    /// @brief Constructs an empty storage.
    ///
    /// Prefer a scope wrapper, including in tests; see @ref resource_scopes_parameter "choosing scopes".
    explicit ResourceScopeStorage(UnsafeManualResourceScopes) noexcept {}

    ResourceScopeStorage(ResourceScopeStorage&& other) noexcept = default;
    ResourceScopeStorage& operator=(ResourceScopeStorage&& other) noexcept = default;

    /// @brief Registers a functor to register some resource that will be
    /// called after the component is successfully created (including all
    /// class descendants) or after the component creation is emulated in
    /// unit tests. The functor must return a RAII-style handle object
    /// that unregisters the previously registered resource. The returned handle's
    /// destructor is called just before the component destructor is called.
    ///
    /// During construction the callback is queued and runs from @ref AfterConstruction.
    /// If @ref Register is called from another scope's opening callback, or after
    /// @ref AfterConstruction has completed, the new scope is opened immediately.
    /// Its destructor runs in reverse opening-completion order.
    ///
    /// @note A queued callback is not called if the component is not created OR
    /// any previously registered callback throws an exception.
    /// @note if you don't have an existing RAII-ish class, but still want
    /// to do a cleanup, you might want to use @ref utils::FastScopeGuard
    /// to wrap the cleanup function.
    ///
    /// See @ref resource_scopes_parameter "where to obtain this storage" and
    /// @ref resource_scopes_register "using Register directly" for custom resource lifetimes.
    template <std::invocable<> AfterConstructionCallback>
    void Register(AfterConstructionCallback after_construction)
    {
        Register(utils::impl::InternalTag{}, Priority{0}, std::move(after_construction));
    }

    /// @cond
    // For internal use only.
    // Lower values run earlier in AfterConstruction and later in BeforeDestruction.
    using Priority = std::int32_t;

    template <std::invocable<> AfterConstructionCallback>
    void Register(utils::impl::InternalTag, Priority priority, AfterConstructionCallback after_construction)
    {
        using Handle = std::invoke_result_t<AfterConstructionCallback>;
        auto scope = std::make_unique<impl::Scope<Handle>>(std::move(after_construction));
        DoRegister(std::move(scope), priority);
    }
    /// @endcond

    /// @brief Call all registered functors.
    ///
    /// If a functor throws, already constructed resources are unregistered via
    /// @ref BeforeDestruction and the exception is rethrown.
    void AfterConstruction();

    /// @brief Unregister all previously registered resources.
    ///
    /// Also drops factories that have not run @ref AfterConstruction yet,
    /// so captured RAII handles unregister immediately.
    void BeforeDestruction() noexcept;

private:
    enum class State {
        kConstruction,
        kAfterConstruction,
        kReady,
        kBeforeDestruction,
        kDestruction,
    };

    struct ScopeWithPriority {
        Priority priority{0};
        impl::ScopePtr scope;
    };

    void DoRegister(impl::ScopePtr resource_scope, Priority priority);
    void OpenAndKeep(impl::ScopePtr resource_scope);
    void OpenAndClose(impl::ScopePtr resource_scope);
    static void SortByPriority(std::vector<ScopeWithPriority>& scopes) noexcept;

    std::vector<ScopeWithPriority> registered_scopes_;
    std::vector<impl::ScopePtr> initialized_scopes_;
    State state_{State::kConstruction};
};

/// @brief A wrapper that provides @ref utils::ResourceScopeStorage for the wrapped object.
///
/// The wrapped object is passed `utils::ResourceScopeStorage&` as the first argument to the constructor.
/// Prefer this over storing @ref ResourceScopeStorage as a field of the wrapped object itself.
/// Use @ref MakeWithResourceScopes when the caller needs a `std::shared_ptr` to the wrapped object.
template <typename Wrapped>
class WithResourceScopes final {
public:
    /// @brief Constructs the wrapped object and passes the embedded @ref utils::ResourceScopeStorage to it
    /// as the first argument.
    template <typename... Args>
    explicit WithResourceScopes(std::in_place_t, Args&&... args)
        : wrapped_(resource_scope_storage_, std::forward<Args>(args)...)
    {
        resource_scope_storage_.AfterConstruction();
    }

    // Not movable: scopes pin a reference to the wrapped object.
    WithResourceScopes(WithResourceScopes&&) = delete;
    WithResourceScopes& operator=(WithResourceScopes&&) = delete;

    ~WithResourceScopes() { resource_scope_storage_.BeforeDestruction(); }

    /// @brief Returns the wrapped object.
    Wrapped& operator*() & noexcept USERVER_IMPL_LIFETIME_BOUND { return wrapped_; }
    /// @overload
    const Wrapped& operator*() const& noexcept USERVER_IMPL_LIFETIME_BOUND { return wrapped_; }

    /// @brief Returns the wrapped object.
    Wrapped* operator->() noexcept USERVER_IMPL_LIFETIME_BOUND { return &wrapped_; }
    /// @overload
    const Wrapped* operator->() const noexcept USERVER_IMPL_LIFETIME_BOUND { return &wrapped_; }

private:
    ResourceScopeStorage resource_scope_storage_{UnsafeManualResourceScopes{}};
    Wrapped wrapped_;
};

/// @brief Constructs @ref WithResourceScopes and returns an aliasing `std::shared_ptr` to the wrapped object.
///
/// The returned pointer shares ownership of the wrapper. @ref WithResourceScopes is not movable
/// and stays at a stable heap address, so scoped registrations remain valid for the lifetime
/// of any copy of the `shared_ptr`.
///
/// @snippet core/src/components/resource_scopes_test.cpp MakeWithResourceScopes
template <typename Wrapped, typename... Args>
std::shared_ptr<Wrapped> MakeWithResourceScopes(Args&&... args) {
    auto holder = std::make_shared<WithResourceScopes<Wrapped>>(std::in_place, std::forward<Args>(args)...);
    auto* const wrapped = std::addressof(**holder);
    return std::shared_ptr<Wrapped>(std::move(holder), wrapped);
}

namespace impl {

struct ResourceScopeStorageHolder {
    ResourceScopeStorage scopes{UnsafeManualResourceScopes{}};
};

template <typename Wrapped>
class UniqueWithResourceScopes final : private ResourceScopeStorageHolder, public Wrapped {
public:
    template <typename... Args>
    explicit UniqueWithResourceScopes(Args&&... args)
        : Wrapped(ResourceScopeStorageHolder::scopes, std::forward<Args>(args)...)
    {
        ResourceScopeStorageHolder::scopes.AfterConstruction();
    }

    UniqueWithResourceScopes(UniqueWithResourceScopes&&) = delete;
    UniqueWithResourceScopes& operator=(UniqueWithResourceScopes&&) = delete;

    ~UniqueWithResourceScopes() override { ResourceScopeStorageHolder::scopes.BeforeDestruction(); }
};

}  // namespace impl

/// @brief Constructs an object with resource scopes and returns an owning `std::unique_ptr` to it.
///
/// The constructor of `Wrapped` receives @ref utils::ResourceScopeStorage as its first argument,
/// followed by the forwarded arguments. `Wrapped` must not be `final` and must have a public virtual destructor:
/// an internal derived wrapper closes the scopes before the destructor of `Wrapped` runs, including when
/// deleted through a pointer to a base class of `Wrapped`.
///
/// Scopes open after the constructor of `Wrapped` completes. If that constructor throws, queued scopes are not
/// opened. If opening a scope throws, already opened scopes close before `Wrapped` is destroyed.
/// Use @ref utils::WithResourceScopes or @ref utils::MakeWithResourceScopes for final types.
///
/// @snippet core/src/components/resource_scopes_test.cpp MakeUniqueWithResourceScopesSnippet
template <typename Wrapped, typename... Args>
std::unique_ptr<Wrapped> MakeUniqueWithResourceScopes(Args&&... args) {
    static_assert(
        !std::is_final_v<Wrapped>,
        "utils::MakeUniqueWithResourceScopes<T> requires a non-final T; use utils::WithResourceScopes or "
        "utils::MakeWithResourceScopes for final types"
    );
    static_assert(
        std::has_virtual_destructor_v<Wrapped>,
        "utils::MakeUniqueWithResourceScopes<T> requires T to have a virtual destructor for deletion through "
        "std::unique_ptr<T>"
    );
    if constexpr (!std::is_final_v<Wrapped> && std::has_virtual_destructor_v<Wrapped>) {
        return std::make_unique<impl::UniqueWithResourceScopes<Wrapped>>(std::forward<Args>(args)...);
    } else {
        return nullptr;
    }
}

ResourceScopeStorage&
LocateDependency(components::WithType<ResourceScopeStorage>, const components::ComponentConfig& config, const components::ComponentContext&);

}  // namespace utils

USERVER_NAMESPACE_END
