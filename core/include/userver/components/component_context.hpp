#pragma once

/// @file userver/components/component_context.hpp
/// @brief @copybrief components::ComponentContext

#include <functional>
#include <memory>
#include <stdexcept>
#include <string>
#include <string_view>
#include <vector>

#include <userver/compiler/demangle.hpp>
#include <userver/components/impl/component_base.hpp>
#include <userver/engine/task/task_processor_fwd.hpp>

USERVER_NAMESPACE_BEGIN

namespace engine::impl {
class TaskContext;
}  // namespace engine::impl

namespace components {

class Manager;

namespace impl {

enum class ComponentLifetimeStage;
class ComponentInfo;

template <class T>
constexpr auto NameFromComponentType() -> decltype(std::string_view{T::kName}) {
  return T::kName;
}

template <class T, class... Args>
constexpr auto NameFromComponentType(Args...) {
  static_assert(!sizeof(T),
                "Component does not have a 'kName' member convertible to "
                "std::string_view. You have to explicitly specify the name: "
                "context.FindComponent<T>(name) or "
                "context.FindComponentOptional<T>(name).");
  return std::string_view{};
}

}  // namespace impl

/// @brief Exception that is thrown from
/// components::ComponentContext::FindComponent() if a component load failed.
class ComponentsLoadCancelledException : public std::runtime_error {
 public:
  ComponentsLoadCancelledException();
  explicit ComponentsLoadCancelledException(const std::string& message);
};

/// @brief Class to retrieve other components.
///
/// Only the const member functions of this class are meant for usage in
/// component constructor (because of that this class is always passed as a
/// const reference to the constructors).
///
/// @see @ref userver_components
class ComponentContext final {
 public:
  /// @brief Finds a component of type T with specified name (if any) and
  /// returns the component after it was initialized.
  ///
  /// Can only be called from other component's constructor in a task where
  /// that constructor was called.
  /// May block and asynchronously wait for the creation of the requested
  /// component.
  /// @throw ComponentsLoadCancelledException if components loading was
  /// cancelled due to errors in the creation of other component.
  /// @throw std::runtime_error if component missing in `component_list` was
  /// requested.
  template <typename T>
  T& FindComponent() const {
    return FindComponent<T>(impl::NameFromComponentType<T>());
  }

  /// @overload T& FindComponent()
  template <typename T>
  T& FindComponent(std::string_view name) const {
    if (!Contains(name)) {
      ThrowNonRegisteredComponent(name, compiler::GetTypeName<T>());
    }

    auto* component_base = DoFindComponent(name);
    T* ptr = dynamic_cast<T*>(component_base);
    if (!ptr) {
      ThrowComponentTypeMismatch(name, compiler::GetTypeName<T>(),
                                 component_base);
    }

    return *ptr;
  }

  template <typename T>
  T& FindComponent(std::string_view /*name*/ = {}) {
    return ReportMisuse<T>();
  }

  /// @brief If there's no component with specified type and name return
  /// nullptr; otherwise behaves as FindComponent().
  template <typename T>
  T* FindComponentOptional() const {
    return FindComponentOptional<T>(impl::NameFromComponentType<T>());
  }

  /// @overload T* FindComponentOptional()
  template <typename T>
  T* FindComponentOptional(std::string_view name) const {
    if (!Contains(name)) {
      return nullptr;
    }
    return dynamic_cast<T*>(DoFindComponent(name));
  }

  template <typename T>
  T& FindComponentOptional(std::string_view /*name*/ = {}) {
    return ReportMisuse<T>();
  }

  /// @brief Returns an engine::TaskProcessor with the specified name.
  engine::TaskProcessor& GetTaskProcessor(const std::string& name) const;

  template <typename T>
  engine::TaskProcessor& GetTaskProcessor(const T&) {
    return ReportMisuse<T>();
  }

  const Manager& GetManager() const;

  /// @returns true if one of the components is in fatal state and can not
  /// work. A component is in fatal state if the
  /// components::ComponentHealth::kFatal value is returned from the overridden
  /// components::LoggableComponentBase::GetComponentHealth().
  bool IsAnyComponentInFatalState() const;

  /// @returns true if there is a component with the specified name and it
  /// could be found via FindComponent()
  bool Contains(std::string_view name) const noexcept;

  template <typename T>
  bool Contains(const T&) {
    return ReportMisuse<T>();
  }

 private:
  template <class T>
  decltype(auto) ReportMisuse() {
    static_assert(!sizeof(T),
                  "components::ComponentContext should be accepted by "
                  "a constant reference, i.e. "
                  "`MyComponent(const components::ComponentConfig& config, "
                  "const components::ComponentContext& context)`");
    return 0;
  }

  friend class Manager;

  ComponentContext() noexcept;

  void Emplace(const Manager& manager,
               std::vector<std::string>&& loading_component_names);

  void Reset() noexcept;

  ~ComponentContext();

  using ComponentFactory =
      std::function<std::unique_ptr<components::impl::ComponentBase>(
          const components::ComponentContext&)>;

  impl::ComponentBase* AddComponent(std::string_view name,
                                    const ComponentFactory& factory);

  void OnAllComponentsLoaded();

  void OnAllComponentsAreStopping();

  void ClearComponents();

  void CancelComponentsLoad();

  [[noreturn]] void ThrowNonRegisteredComponent(std::string_view name,
                                                std::string_view type) const;
  [[noreturn]] void ThrowComponentTypeMismatch(
      std::string_view name, std::string_view type,
      impl::ComponentBase* component) const;

  impl::ComponentBase* DoFindComponent(std::string_view name) const;

  struct Impl;
  std::unique_ptr<Impl> impl_;
};

}  // namespace components

USERVER_NAMESPACE_END
