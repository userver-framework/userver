#pragma once

/// @file userver/testsuite/component_control.hpp
/// @brief @copybrief testsuite::ComponentControl
/// @deprecated Use `userver/testsuite/cache_control.hpp` instead.

#include <functional>
#include <list>
#include <type_traits>

#include <userver/engine/mutex.hpp>

USERVER_NAMESPACE_BEGIN

namespace components::impl {
class ComponentBase;
}  // namespace components::impl

namespace testsuite {

/// @brief Component control interface for testsuite
/// @deprecated Use testsuite::CacheControl instead.
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

/// @deprecated Use testsuite::CacheControl and
/// testsuite::CacheResetRegistration instead.
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
        std::is_base_of_v<components::impl::ComponentBase, T>,
        "ComponentInvalidatorHolder can only be used with components");
  }

  ComponentInvalidatorHolder(const ComponentInvalidatorHolder&) = delete;
  ComponentInvalidatorHolder(ComponentInvalidatorHolder&&) = delete;
  ComponentInvalidatorHolder& operator=(const ComponentInvalidatorHolder&) =
      delete;
  ComponentInvalidatorHolder& operator=(ComponentInvalidatorHolder&&) = delete;
  ~ComponentInvalidatorHolder();

 private:
  ComponentInvalidatorHolder(ComponentControl& component_control,
                             std::function<void()>&& callback);

  ComponentControl& component_control_;
  ComponentControl::InvalidatorHandle invalidator_handle_;
};

}  // namespace testsuite

USERVER_NAMESPACE_END
