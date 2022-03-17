#include <cache/cache_dependencies.hpp>

#include <userver/components/component.hpp>
#include <userver/components/dump_configurator.hpp>
#include <userver/components/statistics_storage.hpp>
#include <userver/dynamic_config/storage/component.hpp>
#include <userver/testsuite/testsuite_support.hpp>

USERVER_NAMESPACE_BEGIN

namespace cache {

namespace {

std::optional<dump::Config> ParseOptionalDumpConfig(
    const components::ComponentConfig& config,
    const components::ComponentContext& context) {
  if (!config.HasMember(dump::kDump)) return {};
  return dump::Config{
      config.Name(), config[dump::kDump],
      context.FindComponent<components::DumpConfigurator>().GetDumpRoot()};
}

engine::TaskProcessor& FindTaskProcessor(
    const components::ComponentContext& context, const Config& static_config) {
  return static_config.task_processor_name
             ? context.GetTaskProcessor(*static_config.task_processor_name)
             : engine::current_task::GetTaskProcessor();
}

std::optional<dynamic_config::Source> FindDynamicConfig(
    const components::ComponentContext& context, const Config& static_config) {
  return static_config.config_updates_enabled
             ? std::optional{context.FindComponent<components::DynamicConfig>()
                                 .GetSource()}
             : std::nullopt;
}

}  // namespace

CacheDependencies CacheDependencies::Make(
    const components::ComponentConfig& config,
    const components::ComponentContext& context) {
  const std::optional<dump::Config> dump_config =
      ParseOptionalDumpConfig(config, context);
  const Config static_config{config, dump_config};

  return CacheDependencies{
      config.Name(),
      static_config,
      FindTaskProcessor(context, static_config),
      FindDynamicConfig(context, static_config),
      context.FindComponent<components::StatisticsStorage>().GetStorage(),
      context.FindComponent<components::TestsuiteSupport>().GetCacheControl(),
      dump_config,
      dump_config ? dump::CreateOperationsFactory(*dump_config, context)
                  : nullptr,
      dump_config ? &context.GetTaskProcessor(dump_config->fs_task_processor)
                  : nullptr,
      context.FindComponent<components::TestsuiteSupport>().GetDumpControl(),
  };
}

}  // namespace cache

USERVER_NAMESPACE_END
