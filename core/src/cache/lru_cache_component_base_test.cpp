#include <cache/lru_cache_component_base_test.hpp>

#include <components/component_list_test.hpp>
#include <userver/components/minimal_component_list.hpp>
#include <userver/components/run.hpp>
#include <userver/fs/blocking/temp_directory.hpp>
#include <userver/fs/blocking/write.hpp>
#include <userver/testsuite/testsuite_support.hpp>

#include <gtest/gtest.h>

USERVER_NAMESPACE_BEGIN
namespace {

// BEWARE! No separate fs-task-processor. Testing almost single thread mode
constexpr std::string_view kStaticConfig = R"(
components_manager:
  default_task_processor: main-task-processor
  fs_task_processor: main-task-processor
  event_thread_pool:
    threads: 1
  task_processors:
    main-task-processor:
      worker_threads: 1
  components:
# /// [Sample lru cache component config]
# yaml
    example-cache:
      size: 1
      ways: 1
      lifetime: 1s # 0 (unlimited) by default
      config-settings: false # true by default
# /// [Sample lru cache component config]
    logging:
      fs-task-processor: main-task-processor
      loggers:
        default:
          file_path: '@null'
    testsuite-support:
)";

}  // namespace

TEST_F(ComponentList, LruCacheComponentSample) {
    const auto temp_root = fs::blocking::TempDirectory::Create();

    /// [Sample lru cache component registration]
    auto component_list = components::MinimalComponentList();
    component_list.Append<ExampleCacheComponent>();
    /// [Sample lru cache component registration]
    component_list.Append<components::TestsuiteSupport>();

    components::RunOnce(components::InMemoryConfig{kStaticConfig}, component_list);
}

USERVER_NAMESPACE_END
