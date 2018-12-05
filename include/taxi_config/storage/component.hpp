#pragma once

#include <chrono>
#include <memory>

#include <components/cache_update_trait.hpp>
#include <components/caching_component_base.hpp>
#include <components/component_config.hpp>
#include <components/component_context.hpp>
#include <taxi_config/config.hpp>

namespace components {

class TaxiConfigImpl;

class TaxiConfig : public LoggableComponentBase,
                   public utils::AsyncEventChannel<
                       const std::shared_ptr<taxi_config::Config>&> {
 public:
  static constexpr const char* kName = "taxi-config";

  TaxiConfig(const ComponentConfig&, const ComponentContext&);
  ~TaxiConfig();

  /// Get config, may block if no config is available yet
  std::shared_ptr<taxi_config::Config> Get() const;

  /// Get config, always returns something without blocking
  /// (either up-to-date config or bootstrap config)
  std::shared_ptr<taxi_config::BootstrapConfig> GetBootstrap() const;

  /// Set up-to-date config. Must be used by config updaters only
  /// (e.g. config client).
  void SetConfig(std::shared_ptr<taxi_config::DocsMap> value_ptr);

  void SetLoadingFailed();

 private:
  bool IsFsCacheEnabled() const;
  void ReadFsCache();
  void WriteFsCache(const taxi_config::DocsMap&);

  void ReadBootstrap(const std::string& bootstrap_fname);

  void DoSetConfig(const std::shared_ptr<taxi_config::DocsMap>& value_ptr);

 private:
  std::shared_ptr<taxi_config::BootstrapConfig> bootstrap_config_;
  bool config_load_cancelled_;

  engine::TaskProcessor& fs_task_processor_;
  const std::string fs_cache_path_;

  mutable engine::ConditionVariable loaded_cv_;
  mutable engine::Mutex loaded_mutex_;
  utils::SwappingSmart<taxi_config::Config> cache_;
};

}  // namespace components
