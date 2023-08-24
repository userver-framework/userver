#pragma once

/// @file userver/testsuite/component_control.hpp
/// @brief @copybrief testsuite::ComponentControl

#include <functional>
#include <list>
#include <type_traits>

#include <userver/components/loggable_component_base.hpp>
#include <userver/engine/mutex.hpp>

USERVER_NAMESPACE_BEGIN

namespace testsuite {

/// @brief Component control interface for testsuite
/// @details All methods are coro-safe.
class ComponentControl final {
 public:
  void InvalidateComponents();

 private:
  friend class ComponentInvalidatorHolder;

  using Callback = std::function<void()>;
  using InvalidatorList = std::list<Callback>;
  using InvalidatorHandle = InvalidatorList::iterator;

  InvalidatorHandle RegisterComponentInvalidator(Callback&& callback);

  void UnregisterComponentInvalidator(InvalidatorHandle handle) noexcept;

  engine::Mutex mutex_;
  InvalidatorList invalidators_;
};

/// RAII helper for testsuite registration
class ComponentInvalidatorHolder final {
 public:
  template <class T>
  ComponentInvalidatorHolder(ComponentControl& component_control, T& component,
                             void (T::*invalidate_method)())
      : ComponentInvalidatorHolder(component_control,
                                   [&component, invalidate_method] {
                                     (component.*invalidate_method)();
                                   }) {
    static_assert(
        std::is_base_of_v<components::LoggableComponentBase, T>,
        "ComponentInvalidatorHolder can only be used with components");
  }

  ~ComponentInvalidatorHolder();

  ComponentInvalidatorHolder(const ComponentInvalidatorHolder&) = delete;
  ComponentInvalidatorHolder(ComponentInvalidatorHolder&&) = delete;
  ComponentInvalidatorHolder& operator=(const ComponentInvalidatorHolder&) =
      delete;
  ComponentInvalidatorHolder& operator=(ComponentInvalidatorHolder&&) = delete;

 private:
  ComponentInvalidatorHolder(ComponentControl& component_control,
                             std::function<void()>&& callback);

  ComponentControl& component_control_;
  ComponentControl::InvalidatorHandle invalidator_handle_;
};

}  // namespace testsuite

USERVER_NAMESPACE_END
