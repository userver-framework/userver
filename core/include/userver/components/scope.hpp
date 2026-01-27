#pragma once

/// @file userver/components/scope.hpp
/// @brief @copybrief components::MakeScope

#include <functional>
#include <memory>
#include <optional>

#include <userver/utils/assert.hpp>

USERVER_NAMESPACE_BEGIN

namespace components {

namespace impl {
class ScopeBase {
public:
    virtual ~ScopeBase() = default;

    virtual void AfterConstruction() = 0;
};

template <typename Handle>
class Scope final : public ScopeBase {
public:
    using AfterConstructionCallback = std::function<Handle()>;

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
    using AfterConstructionCallback = std::function<void()>;

    explicit Scope(AfterConstructionCallback after_construction)
        : after_construction_(std::move(after_construction))
    {}

    void AfterConstruction() override { after_construction_(); }

private:
    AfterConstructionCallback after_construction_;
};

}  // namespace impl

/// @brief An object of ScopePtr defines actions to do after
/// a component is constructed and just before it is destroyed.
///
/// @see @ref components::ComponentContext::RegisterScope
using ScopePtr = std::unique_ptr<impl::ScopeBase>;

/// @brief Constructs an object of type @ref ScopePtr from
/// a registration callback. The callback must return an object that undoes
/// the registration in its destructor.
///
/// @see @ref components::ComponentContext::RegisterScope
template <typename AfterConstructionCallback>
ScopePtr MakeScope(AfterConstructionCallback after_construction)
{
    using Handle = std::invoke_result_t<AfterConstructionCallback>;
    return std::make_unique<impl::Scope<Handle>>(std::move(after_construction));
}

}  // namespace components

USERVER_NAMESPACE_END
