#pragma once

/// @file userver/components/component_list.hpp
/// @brief @copybrief components::ComponentList

#include <functional>
#include <memory>
#include <set>
#include <string>

#include <userver/components/component_fwd.hpp>
#include <userver/components/static_config_validator.hpp>

USERVER_NAMESPACE_BEGIN

namespace components {

namespace impl {

template <class T>
auto NameRegistrationFromComponentType() -> decltype(std::string_view{T::kName}) {
    return std::string_view{T::kName};
}

template <class T, class... Args>
auto NameRegistrationFromComponentType(Args...) {
    static_assert(
        !sizeof(T),
        "Component does not have a 'kName' member convertible to "
        "std::string_view. You have to explicitly specify the name: "
        "component_list.Append<T>(name)."
    );
    return std::string_view{};
}

class ComponentAdderBase {
public:
    ComponentAdderBase() = delete;
    ComponentAdderBase(const ComponentAdderBase&) = delete;
    ComponentAdderBase(ComponentAdderBase&&) = delete;
    ComponentAdderBase& operator=(const ComponentAdderBase&) = delete;
    ComponentAdderBase& operator=(ComponentAdderBase&&) = delete;

    ComponentAdderBase(std::string name, ConfigFileMode config_file_mode);

    virtual ~ComponentAdderBase();

    const std::string& GetComponentName() const noexcept { return name_; }

    ConfigFileMode GetConfigFileMode() const { return config_file_mode_; }

    virtual std::unique_ptr<RawComponentBase>
    MakeComponent(const ComponentConfig& config, const ComponentContext& context) const = 0;

    virtual void ValidateStaticConfig(const ComponentConfig&, ValidationMode) const = 0;

    virtual yaml_config::Schema GetStaticConfigSchema() const = 0;

private:
    std::string name_;
    ConfigFileMode config_file_mode_;
};

template <typename Component>
class ComponentAdder final : public ComponentAdderBase {
public:
    // Using std::is_convertible_v because std::is_base_of_v returns true even
    // if RawComponentBase is a private, protected, or ambiguous base class.
    static_assert(
        std::is_convertible_v<Component*, components::RawComponentBase*>,
        "Component should publicly inherit from components::ComponentBase"
        " and the component definition should be visible at its registration"
    );

    explicit ComponentAdder(std::string name) : ComponentAdderBase(std::move(name), kConfigFileMode<Component>) {}

    std::unique_ptr<RawComponentBase> MakeComponent(const ComponentConfig& config, const ComponentContext& context)
        const override {
        return std::make_unique<Component>(config, context);
    }

    void ValidateStaticConfig(const ComponentConfig& static_config, ValidationMode validation_mode) const override {
        impl::TryValidateStaticConfig<Component>(GetComponentName(), static_config, validation_mode);
    }

    yaml_config::Schema GetStaticConfigSchema() const override { return impl::GetStaticConfigSchema<Component>(); }
};

using ComponentAdderPtr = std::unique_ptr<const impl::ComponentAdderBase>;

struct ComponentAdderComparator {
    using is_transparent = std::true_type;

    static std::string_view ToStringView(const ComponentAdderPtr& x) noexcept {
        return std::string_view{x->GetComponentName()};
    }

    static std::string_view ToStringView(std::string_view x) noexcept { return x; }

    template <class T, class U>
    bool operator()(const T& x, const U& y) const noexcept {
        return ToStringView(x) < ToStringView(y);
    }
};

}  // namespace impl

/// @brief A list to keep a unique list of components to start with
/// components::Run(), utils::DaemonMain() or components::RunOnce().
class ComponentList final {
public:
    /// Appends a component with default component name Component::kName.
    template <typename Component>
    ComponentList& Append() &;

    /// Appends a component with a provided component name.
    template <typename Component>
    ComponentList& Append(std::string_view name) &;

    /// Merges components from `other` into `*this`.
    ComponentList& AppendComponentList(ComponentList&& other) &;

    /// @overload
    ComponentList&& AppendComponentList(ComponentList&& other) &&;

    /// @overload
    template <typename Component, typename... Args>
    ComponentList&& Append(Args&&...) &&;

    /// @return true iff the component with provided name was added to *this.
    bool Contains(std::string_view name) const { return adders_.count(name) > 0; }

    /// @cond
    using Adders = std::set<impl::ComponentAdderPtr, impl::ComponentAdderComparator>;

    Adders::const_iterator begin() const { return adders_.begin(); }
    Adders::const_iterator end() const { return adders_.end(); }

    ComponentList& Append(impl::ComponentAdderPtr&& added) &;

    yaml_config::Schema GetStaticConfigSchema() const;
    /// @endcond

private:
    Adders adders_;
};

template <typename Component>
ComponentList& ComponentList::Append() & {
    return Append<Component>(impl::NameRegistrationFromComponentType<Component>());
}

template <typename Component>
ComponentList& ComponentList::Append(std::string_view name) & {
    using Adder = impl::ComponentAdder<Component>;
    auto adder = std::make_unique<const Adder>(std::string{name});
    return Append(std::move(adder));
}

template <typename Component, typename... Args>
ComponentList&& ComponentList::Append(Args&&... args) && {
    return std::move(Append<Component>(std::forward<Args>(args)...));
}

}  // namespace components

USERVER_NAMESPACE_END
