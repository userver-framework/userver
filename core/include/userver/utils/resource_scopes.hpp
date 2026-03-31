#pragma once

/// @file userver/utils/scope.hpp
/// @brief @copybrief utils::ResourceScopeStorage

#include <functional>
#include <memory>
#include <optional>

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

/// @brief Smart collection of @ref ScopePtr.
/// It is a helper class used in component system or in a component-less
/// unit tests.
class ResourceScopeStorage final {
public:
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
    template <typename AfterConstructionCallback>
    void Register(AfterConstructionCallback after_construction)
    {
        using Handle = std::invoke_result_t<AfterConstructionCallback>;
        auto scope = std::make_unique<impl::Scope<Handle>>(std::move(after_construction));
        DoRegister(std::move(scope));
    }

    /// @brief Call all registered functors.
    void AfterConstruction();

    /// @brief Free all unregister previously rgistered resources.
    void BeforeDestruction();

private:
    void DoRegister(impl::ScopePtr resource_scope);

    std::vector<impl::ScopePtr> registered_scopes_;
    std::vector<impl::ScopePtr> initialized_scopes_;
    bool scope_registration_finished_{false};
};

ResourceScopeStorage&
LocateDependency(components::WithType<ResourceScopeStorage>, const components::ComponentConfig& config, const components::ComponentContext&);

}  // namespace utils

USERVER_NAMESPACE_END
