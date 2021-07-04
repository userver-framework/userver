#include <cache/internal_test_helpers.hpp>

#include <engine/task/task_processor.hpp>
#include <userver/components/component_config.hpp>
#include <userver/dump/config.hpp>
#include <userver/dump/factory.hpp>
#include <userver/yaml_config/yaml_config.hpp>

namespace cache {

namespace {

components::ComponentConfig MakeComponentConfig(yaml_config::YamlConfig config,
                                                std::string_view name) {
  components::ComponentConfig result{std::move(config)};
  result.SetName(std::string{name});
  return result;
}

}  // namespace

CacheMockBase::CacheMockBase(std::string_view name,
                             const yaml_config::YamlConfig& config,
                             testsuite::CacheControl& cache_control)
    : CacheUpdateTrait(Config{MakeComponentConfig(config, name), std::nullopt},
                       std::string{name}, cache_control, std::nullopt, nullptr,
                       nullptr, nullptr) {}

DumpableCacheMockBase::DumpableCacheMockBase(
    std::string_view name, const yaml_config::YamlConfig& config,
    const fs::blocking::TempDirectory& dump_root,
    testsuite::CacheControl& cache_control,
    testsuite::DumpControl& dump_control)
    : DumpableCacheMockBase(name, config,
                            dump::Config{std::string{name}, config[dump::kDump],
                                         dump_root.GetPath()},
                            cache_control, dump_control) {}

DumpableCacheMockBase::DumpableCacheMockBase(
    std::string_view name, const yaml_config::YamlConfig& config,
    const dump::Config& dump_config, testsuite::CacheControl& cache_control,
    testsuite::DumpControl& dump_control)
    : CacheUpdateTrait(Config{MakeComponentConfig(config, name), dump_config},
                       std::string{name}, cache_control, dump_config,
                       dump::CreateDefaultOperationsFactory(dump_config),
                       &engine::current_task::GetTaskProcessor(),
                       &dump_control) {}

MockError::MockError() : std::runtime_error("Simulating an update error") {}

}  // namespace cache
