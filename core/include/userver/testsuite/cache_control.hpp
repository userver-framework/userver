#pragma once

/// @file userver/testsuite/cache_control.hpp
/// @brief @copybrief testsuite::CacheControl

#include <atomic>
#include <functional>
#include <list>
#include <string>
#include <type_traits>
#include <unordered_set>
#include <utility>

#include <userver/cache/update_type.hpp>
#include <userver/concurrent/variable.hpp>
#include <userver/utils/assert.hpp>

USERVER_NAMESPACE_BEGIN

namespace cache {
class CacheUpdateTrait;
struct Config;
}  // namespace cache

namespace components {
class ComponentContext;
}  // namespace components

namespace components::impl {
class ComponentBase;
}  // namespace components::impl

namespace testsuite {

namespace impl {
enum class PeriodicUpdatesMode { kDefault, kEnabled, kDisabled };
}  // namespace impl

class CacheResetRegistration;

/// @brief Testsuite interface for caches and cache-like components.
///
/// If a component stores transient state that may be carried between tests,
/// or stores caches that may become stale, then it should register its resetter
/// here. Example:
///
/// @snippet testsuite/cache_control_test.cpp  sample
///
/// Testsuite will then call this hook in the beginning of each test.
/// You can also reset a specific cache in testsuite explicitly as follows:
///
/// @code
/// service_client.invalidate_caches(names=['your-cache-name'])
/// @endcode
///
/// CacheControl is normally acquired through testsuite::FindCacheControl.
///
/// All methods are coro-safe.
class CacheControl final {
 public:
  /// @brief Register a cache reset function. The returned handle must be kept
  /// alive to keep supporting cache resetting.
  ///
  /// @warning The function should be called in the component's constructor
  /// *after* all FindComponent calls. This ensures that reset will first be
  /// called for dependencies, then for dependent components.
  template <typename Component>
  CacheResetRegistration RegisterCache(Component* self, std::string_view name,
                                       void (Component::*reset_method)());

  /// @brief Reset all the registered caches.
  ///
  /// @a update_type is used by caches derived from
  /// @a component::CachingComponentBase.
  void ResetAllCaches(
      cache::UpdateType update_type,
      const std::unordered_set<std::string>& force_incremental_names);

  /// @brief Reset caches with the specified @a names.
  ///
  /// @a update_type is used by caches derived from
  /// @a component::CachingComponentBase.
  void ResetCaches(
      cache::UpdateType update_type,
      std::unordered_set<std::string> reset_only_names,
      const std::unordered_set<std::string>& force_incremental_names);

  /// @cond
  // For internal use only.
  explicit CacheControl(impl::PeriodicUpdatesMode);

  CacheControl(CacheControl&&) = delete;
  CacheControl& operator=(CacheControl&&) = delete;

  // For internal use only.
  bool IsPeriodicUpdateEnabled(const cache::Config& cache_config,
                               const std::string& cache_name) const;

  // For internal use only.
  CacheResetRegistration RegisterPeriodicCache(cache::CacheUpdateTrait& cache);

 private:
  friend class CacheResetRegistration;

  struct CacheInfo final {
    std::string name;
    std::function<void(cache::UpdateType)> reset;
    bool needs_span{true};
  };

  using CacheInfoIterator = std::list<CacheInfo>::iterator;

  CacheInfoIterator DoRegisterCache(CacheInfo&&);

  void UnregisterCache(CacheInfoIterator) noexcept;

  static void DoResetCache(const CacheInfo& info,
                           cache::UpdateType update_type);

  const impl::PeriodicUpdatesMode periodic_updates_mode_;
  concurrent::Variable<std::list<CacheInfo>> caches_;
};

/// @brief RAII helper for testsuite registration. Must be kept alive to keep
/// supporting cache resetting.
/// @warning Make sure to always place CacheResetRegistration after the rest of
/// the component's fields.
/// @see testsuite::CacheControl
class [[nodiscard]] CacheResetRegistration final {
 public:
  CacheResetRegistration() noexcept;

  CacheResetRegistration(CacheResetRegistration&&) noexcept;
  CacheResetRegistration& operator=(CacheResetRegistration&&) noexcept;
  ~CacheResetRegistration();

  /// Unregister the cache component explicitly.
  /// `Unregister` is called in the destructor automatically.
  void Unregister() noexcept;

  /// @cond
  // For internal use only.
  CacheResetRegistration(CacheControl&, CacheControl::CacheInfoIterator);
  /// @endcond

 private:
  CacheControl* cache_control_{nullptr};
  CacheControl::CacheInfoIterator cache_info_iterator_{};
};

/// The method for acquiring testsuite::CacheControl in the component system.
CacheControl& FindCacheControl(const components::ComponentContext& context);

template <typename Component>
CacheResetRegistration CacheControl::RegisterCache(
    Component* self, std::string_view name, void (Component::*reset_method)()) {
  static_assert(std::is_base_of_v<components::impl::ComponentBase, Component>,
                "CacheControl can only be used with components");
  UASSERT(self);
  UASSERT(reset_method);
  auto iter = DoRegisterCache(CacheInfo{
      /*name=*/std::string{name},
      /*reset=*/
      [self, reset_method]([[maybe_unused]] cache::UpdateType) {
        (self->*reset_method)();
      },
      /*needs_span=*/true,
  });
  return CacheResetRegistration(*this, std::move(iter));
}

}  // namespace testsuite

USERVER_NAMESPACE_END
