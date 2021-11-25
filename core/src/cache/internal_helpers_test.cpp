#include <cache/internal_test_helpers.hpp>

#include <engine/task/task_processor.hpp>
#include <userver/components/component.hpp>
#include <userver/dump/config.hpp>
#include <userver/dump/factory.hpp>

USERVER_NAMESPACE_BEGIN

namespace cache {

CacheMockBase::CacheMockBase(std::string_view name,
                             const yaml_config::YamlConfig& config,
                             MockEnvironment& environment)
    : CacheMockBase(
          name, config, environment,
          config.HasMember(dump::kDump)
              ? std::optional<dump::Config>(std::in_place, std::string{name},
                                            config[dump::kDump],
                                            environment.dump_root.GetPath())
              : std::nullopt) {}

CacheMockBase::CacheMockBase(std::string_view name,
                             const yaml_config::YamlConfig& config,
                             MockEnvironment& environment,
                             const std::optional<dump::Config>& dump_config)
    : CacheUpdateTrait(
          Config{config, dump_config}, std::string{name},
          engine::current_task::GetTaskProcessor(),
          environment.config_storage.GetSource(),
          environment.statistics_storage, environment.cache_control,
          dump_config,
          dump_config ? dump::CreateDefaultOperationsFactory(*dump_config)
                      : nullptr,
          &engine::current_task::GetTaskProcessor(), environment.dump_control) {
}

MockError::MockError() : std::runtime_error("Simulating an update error") {}

}  // namespace cache

USERVER_NAMESPACE_END
