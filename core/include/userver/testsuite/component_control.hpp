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

  struct Invalidator {
    Invalidator(components::LoggableComponentBase&, Callback);

    components::LoggableComponentBase* owner;
    Callback callback;
  };

  void RegisterComponentInvalidator(components::LoggableComponentBase& owner,
                                    Callback callback);

  void UnregisterComponentInvalidator(components::LoggableComponentBase& owner);

  engine::Mutex mutex_;
  std::list<Invalidator> invalidators_;
};

/// RAII helper for testsuite registration
class ComponentInvalidatorHolder final {
 public:
  template <class T>
  ComponentInvalidatorHolder(ComponentControl& component_control, T& component,
                             void (T::*invalidate_method)())
      : component_control_(component_control), component_(component) {
    static_assert(
        std::is_base_of<components::LoggableComponentBase, T>::value,
        "ComponentInvalidatorHolder can only be used with components");

    component_control_.RegisterComponentInvalidator(
        component_, [&component, invalidate_method]() {
          (component.*invalidate_method)();
        });
  }

  ~ComponentInvalidatorHolder();

  ComponentInvalidatorHolder(const ComponentInvalidatorHolder&) = delete;
  ComponentInvalidatorHolder(ComponentInvalidatorHolder&&) = delete;
  ComponentInvalidatorHolder& operator=(const ComponentInvalidatorHolder&) =
      delete;
  ComponentInvalidatorHolder& operator=(ComponentInvalidatorHolder&&) = delete;

 private:
  ComponentControl& component_control_;
  components::LoggableComponentBase& component_;
};

}  // namespace testsuite

USERVER_NAMESPACE_END
