#pragma once

/// @file userver/utils/resource_scopes.hpp
/// @brief @copybrief utils::ResourceScopeStorage

#include <concepts>
#include <memory>
#include <optional>

#include <userver/compiler/impl/lifetime.hpp>
#include <userver/components/component_fwd.hpp>
#include <userver/utils/assert.hpp>
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

/// @brief Defers subscription and callback registration until the object is fully constructed.
///
/// Components often register external subscriptions (statistics writers, config listeners,
/// and similar) that capture `this` and run later on another thread. Registering them
/// directly in the constructor is unsafe: the callback may fire before the constructor
/// finishes and observe partially initialized fields. Unregistering in the destructor
/// is equally unsafe if the callback can still run while members are already being
/// destroyed.
///
/// During construction, call @ref Register to queue a functor that performs the actual
/// registration. The component system calls @ref AfterConstruction when the constructor
/// (including derived classes) has completed, and @ref BeforeDestruction before the
/// destructor body runs. That way registration callbacks see a fully built object, and
/// unregistration runs before members used by the callback are torn down.
///
/// The same storage is available from @ref components::ComponentContext::Scopes in
/// components, or as a standalone helper in unit tests and @ref WithResourceScopes.
///
/// @snippet core/src/components/resource_scopes_test.cpp ResourceScopeStorage - HappyPathOrder
class ResourceScopeStorage final {
public:
    ResourceScopeStorage() = default;

    ResourceScopeStorage(ResourceScopeStorage&& other) noexcept = default;
    ResourceScopeStorage& operator=(ResourceScopeStorage&& other) noexcept = default;

    /// @brief Registers a functor to register some resource that will be
    /// called after the component is successfully created (including all
    /// class descendants) or after the component creation is emulated in
    /// unit tests. The functor must return a RAII-style handle object
    /// that unregisters the previously registered resource. The returned handle's
    /// destructor is called just before the component destructor is called.
    ///
    /// @note callback is not called if the component is not created OR
    /// any previously registered callback throws an exception.
    /// @note if you don't have an existing RAII-ish class, but still want
    /// to do a cleanup, you might want to use @ref utils::FastScopeGuard
    /// to wrap the cleanup function.
    template <std::invocable<> AfterConstructionCallback>
    void Register(AfterConstructionCallback after_construction)
    {
        using Handle = std::invoke_result_t<AfterConstructionCallback>;
        auto scope = std::make_unique<impl::Scope<Handle>>(std::move(after_construction));
        DoRegister(std::move(scope));
    }

    /// @brief Call all registered functors.
    void AfterConstruction();

    /// @brief Unregister all previously registered resources.
    void BeforeDestruction();

private:
    void DoRegister(impl::ScopePtr resource_scope);

    std::vector<impl::ScopePtr> registered_scopes_;
    std::vector<impl::ScopePtr> initialized_scopes_;
    bool scope_registration_finished_{false};
};

/// @brief A wrapper that provides @ref utils::ResourceScopeStorage for the wrapped object.
///
/// The wrapped object is passed `utils::ResourceScopeStorage&` as the first argument to the constructor.
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

    WithResourceScopes(WithResourceScopes&& other) noexcept = default;
    WithResourceScopes& operator=(WithResourceScopes&& other) noexcept = default;

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
    ResourceScopeStorage resource_scope_storage_;
    Wrapped wrapped_;
};

ResourceScopeStorage&
LocateDependency(components::WithType<ResourceScopeStorage>, const components::ComponentConfig& config, const components::ComponentContext&);

}  // namespace utils

USERVER_NAMESPACE_END
