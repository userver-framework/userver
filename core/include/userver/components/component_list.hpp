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

class Manager;

namespace impl {

template <class T>
auto NameRegistrationFromComponentType() -> decltype(std::string{T::kName}) {
  return std::string{T::kName};
}

template <class T, class... Args>
auto NameRegistrationFromComponentType(Args...) {
  static_assert(!sizeof(T),
                "Component does not have a 'kName' member convertible to "
                "std::string. You have to explicitly specify the name: "
                "component_list.Append<T>(name).");
  return std::string{};
}

using ComponentBaseFactory =
    std::function<std::unique_ptr<components::impl::ComponentBase>(
        const components::ComponentConfig&,
        const components::ComponentContext&)>;

// Hides manager implementation from header
void AddComponentImpl(Manager& manager,
                      const components::ComponentConfigMap& config_map,
                      const std::string& name, ComponentBaseFactory factory);

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

  virtual void operator()(Manager&,
                          const components::ComponentConfigMap&) const = 0;

  virtual void ValidateStaticConfig(const ComponentConfig&,
                                    ValidationMode) const = 0;

 private:
  std::string name_;
  ConfigFileMode config_file_mode_;
};

template <typename Component>
class ComponentAdder final : public ComponentAdderBase {
 public:
  explicit ComponentAdder(std::string name)
      : ComponentAdderBase(std::move(name), kConfigFileMode<Component>) {}

  void operator()(Manager&,
                  const components::ComponentConfigMap&) const override;

  void ValidateStaticConfig(const ComponentConfig& static_config,
                            ValidationMode validation_mode) const override {
    impl::TryValidateStaticConfig<Component>(static_config, validation_mode);
  }
};

template <typename Component>
void ComponentAdder<Component>::operator()(
    Manager& manager, const components::ComponentConfigMap& config_map) const {
  // Using std::is_convertible_v because std::is_base_of_v returns true even
  // if ComponentBase is a private, protected, or ambiguous base class.
  static_assert(
      std::is_convertible_v<Component*, components::impl::ComponentBase*>,
      "Component should publicly inherit from components::LoggableComponentBase"
      " and the component definition should be visible at its registration");
  impl::AddComponentImpl(manager, config_map, GetComponentName(),
                         [](const components::ComponentConfig& config,
                            const components::ComponentContext& context) {
                           return std::make_unique<Component>(config, context);
                         });
}

using ComponentAdderPtr = std::unique_ptr<const impl::ComponentAdderBase>;

struct ComponentAdderComparator {
  using is_transparent = std::true_type;

  static std::string_view ToStringView(const ComponentAdderPtr& x) noexcept {
    return std::string_view{x->GetComponentName()};
  }

  static std::string_view ToStringView(std::string_view x) noexcept {
    return x;
  }

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
  ComponentList& Append(std::string name) &;

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
  using Adders =
      std::set<impl::ComponentAdderPtr, impl::ComponentAdderComparator>;

  Adders::const_iterator begin() const { return adders_.begin(); }
  Adders::const_iterator end() const { return adders_.end(); }

  ComponentList& Append(impl::ComponentAdderPtr&& added) &;

 private:
  Adders adders_;
  /// @endcond
};

template <typename Component>
ComponentList& ComponentList::Append() & {
  return Append<Component>(
      impl::NameRegistrationFromComponentType<Component>());
}

template <typename Component>
ComponentList& ComponentList::Append(std::string name) & {
  using Adder = impl::ComponentAdder<Component>;
  auto adder = std::make_unique<const Adder>(std::move(name));
  return Append(std::move(adder));
}

template <typename Component, typename... Args>
ComponentList&& ComponentList::Append(Args&&... args) && {
  return std::move(Append<Component>(std::forward<Args>(args)...));
}

}  // namespace components

USERVER_NAMESPACE_END
