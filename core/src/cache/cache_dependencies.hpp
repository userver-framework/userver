#pragma once

#include <memory>
#include <optional>
#include <string>

#include <userver/cache/cache_config.hpp>
#include <userver/components/component_fwd.hpp>
#include <userver/dump/config.hpp>
#include <userver/dump/factory.hpp>
#include <userver/dynamic_config/source.hpp>
#include <userver/engine/task/task_processor_fwd.hpp>
#include <userver/testsuite/cache_control.hpp>
#include <userver/testsuite/dump_control.hpp>
#include <userver/utils/statistics/storage.hpp>

USERVER_NAMESPACE_BEGIN

namespace cache {

struct CacheDependencies final {
  std::string name;
  Config config;
  engine::TaskProcessor& task_processor;
  std::optional<dynamic_config::Source> config_source;
  utils::statistics::Storage& statistics_storage;
  testsuite::CacheControl& cache_control;
  std::optional<dump::Config> dump_config;
  std::unique_ptr<dump::OperationsFactory> dump_rw_factory;
  engine::TaskProcessor* fs_task_processor;
  testsuite::DumpControl& dump_control;

  static CacheDependencies Make(const components::ComponentConfig& config,
                                const components::ComponentContext& context);
};

}  // namespace cache

USERVER_NAMESPACE_END
