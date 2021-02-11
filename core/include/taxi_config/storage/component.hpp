#pragma once

#include <chrono>
#include <exception>
#include <memory>

#include <cache/cache_update_trait.hpp>
#include <components/component_config.hpp>
#include <components/component_context.hpp>
#include <components/loggable_component_base.hpp>
#include <rcu/rcu.hpp>
#include <taxi_config/config.hpp>
#include <taxi_config/config_ptr.hpp>
#include <utils/async_event_channel.hpp>
#include <utils/shared_readable_ptr.hpp>

namespace components {

// NOLINTNEXTLINE(fuchsia-multiple-inheritance)
class TaxiConfig : public LoggableComponentBase,
                   public utils::AsyncEventChannel<
                       const std::shared_ptr<const taxi_config::Config>&> {
 public:
  static constexpr const char* kName = "taxi-config";

  TaxiConfig(const ComponentConfig&, const ComponentContext&);
  ~TaxiConfig() override;

  /// Get config, may block if no config is available yet
  std::shared_ptr<const taxi_config::Config> Get() const;

  template <typename T>
  utils::SharedReadablePtr<T> GetAs() const {
    auto config = Get();
    const T& ptr = config->Get<T>();
    // Use shared_ptr's aliasing constructor
    return std::shared_ptr<const T>{std::move(config), &ptr};
  }

  void NotifyLoadingFailed(const std::string& updater_error);

  /// Get config, always returns something without blocking
  /// (either up-to-date config or bootstrap config)
  std::shared_ptr<const taxi_config::BootstrapConfig> GetBootstrap() const;

  /// Get config, never blocks, may return nullptr
  std::shared_ptr<const taxi_config::Config> GetNoblock() const;

  /// Set up-to-date config. Must be used by config updaters only
  /// (e.g. config client).
  void SetConfig(std::shared_ptr<const taxi_config::DocsMap> value_ptr);

  void OnLoadingCancelled() override;

  template <class Class>
  [[nodiscard]] ::utils::AsyncEventSubscriberScope UpdateAndListen(
      const Class* obj, std::string name,
      void (Class::*func)(const std::shared_ptr<const taxi_config::Config>&)
          const) {
    return UpdateAndListenImpl(obj, std::move(name), func);
  }

  template <class Class>
  [[nodiscard]] ::utils::AsyncEventSubscriberScope UpdateAndListen(
      Class* obj, std::string name,
      void (Class::*func)(const std::shared_ptr<const taxi_config::Config>&)) {
    return UpdateAndListenImpl(obj, std::move(name), func);
  }

 private:
  /// non-blocking check if config is available
  bool Has() const;

  bool IsFsCacheEnabled() const;
  void ReadFsCache();
  void WriteFsCache(const taxi_config::DocsMap&);

  void ReadBootstrap(const std::string& bootstrap_fname);

  void DoSetConfig(
      const std::shared_ptr<const taxi_config::DocsMap>& value_ptr);

 private:
  template <class Class, class Function>
  ::utils::AsyncEventSubscriberScope UpdateAndListenImpl(Class* obj,
                                                         std::string name,
                                                         Function func) {
    // It is fine to loose update because in a few seconds we'll get a new one.
    {
      // Call func before AddListener to avoid concurrent invocation of func.
      auto value = Get();
      (obj->*func)(value);
    }

    return AddListener(obj, std::move(name), func);
  }

  // for cache_
  friend const taxi_config::impl::Storage& taxi_config::impl::FindStorage(
      const components::ComponentContext& context);

  std::shared_ptr<const taxi_config::BootstrapConfig> bootstrap_config_;
  bool config_load_cancelled_;

  engine::TaskProcessor& fs_task_processor_;
  const std::string fs_cache_path_;

  mutable engine::ConditionVariable loaded_cv_;
  mutable engine::Mutex loaded_mutex_;
  taxi_config::impl::Storage cache_;
  std::string loading_error_msg_;
};

}  // namespace components
