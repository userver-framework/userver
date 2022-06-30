#include <cache/internal_helpers_test.hpp>

#include <userver/components/component.hpp>
#include <userver/dump/config.hpp>
#include <userver/dump/factory.hpp>
#include <userver/engine/task/task.hpp>

USERVER_NAMESPACE_BEGIN

namespace cache {

namespace {

CacheDependencies MakeDependencies(std::string_view name,
                                   const yaml_config::YamlConfig& config,
                                   MockEnvironment& environment) {
  const std::optional<dump::Config> dump_config =
      config.HasMember(dump::kDump)
          ? std::optional<dump::Config>(std::in_place, std::string{name},
                                        config[dump::kDump],
                                        environment.dump_root.GetPath())
          : std::nullopt;

  return {
      std::string{name},
      Config{config, dump_config},
      engine::current_task::GetTaskProcessor(),
      environment.config_storage.GetSource(),
      environment.statistics_storage,
      environment.cache_control,
      dump_config,
      dump_config ? dump::CreateDefaultOperationsFactory(*dump_config)
                  : nullptr,
      &engine::current_task::GetTaskProcessor(),
      environment.dump_control,
  };
}

}  // namespace

CacheMockBase::CacheMockBase(std::string_view name,
                             const yaml_config::YamlConfig& config,
                             MockEnvironment& environment)
    : CacheUpdateTrait(MakeDependencies(name, config, environment)) {}

void CacheMockBase::Cleanup() {}

MockError::MockError() : std::runtime_error("Simulating an update error") {}

}  // namespace cache

USERVER_NAMESPACE_END
