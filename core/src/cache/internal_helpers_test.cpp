#include <cache/internal_test_helpers.hpp>

#include <dump/config.hpp>
#include <dump/factory.hpp>
#include <engine/task/task_processor.hpp>

namespace cache {

CacheMockBase::CacheMockBase(const components::ComponentConfig& config,
                             testsuite::CacheControl& cache_control,
                             testsuite::DumpControl& dump_control)
    : CacheUpdateTrait(
          Config{config, dump::Config::ParseOptional(config)}, config.Name(),
          cache_control, dump::Config::ParseOptional(config),
          config.HasMember(dump::kDump)
              ? dump::CreateDefaultOperationsFactory(dump::Config{config})
              : nullptr,
          &engine::current_task::GetTaskProcessor(), dump_control) {}

MockError::MockError() : std::runtime_error("Simulating an update error") {}

}  // namespace cache
