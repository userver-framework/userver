#include <cache/lru_cache_component_base_test.hpp>

#include <components/component_list_test.hpp>
#include <userver/components/minimal_component_list.hpp>
#include <userver/components/run.hpp>
#include <userver/formats/yaml/serialize.hpp>
#include <userver/fs/blocking/temp_directory.hpp>
#include <userver/fs/blocking/write.hpp>
#include <userver/testsuite/testsuite_support.hpp>
#include <userver/yaml_config/impl/validate_static_config.hpp>

#include <gtest/gtest.h>

USERVER_NAMESPACE_BEGIN
namespace {

// BEWARE! No separate fs-task-processor. Testing almost single thread mode
constexpr std::string_view kStaticConfigTemplate = R"(
components_manager:
  coro_pool:
    initial_size: 50
    max_size: 500
  default_task_processor: main-task-processor
  event_thread_pool:
    threads: 1
  task_processors:
    main-task-processor:
      thread_name: main-worker
      worker_threads: 1
  components:
    manager-controller:  # Nothing
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
    tracer:
        service-name: config-service
    statistics-storage:
      # Nothing
    testsuite-support:
      testsuite-periodic-update-enabled: true
      testsuite-pg-execute-timeout: 300ms
      testsuite-pg-statement-timeout: 300ms
      testsuite-pg-readonly-master-expected: false
      testsuite-redis-timeout-connect: 5s
      testsuite-redis-timeout-single: 1s
      testsuite-redis-timeout-all: 750ms
    dynamic-config:
      fs-cache-path: $runtime_config_path
      fs-task-processor: main-task-processor
    dynamic-config-fallbacks:
      fallback-path: $runtime_config_path
config_vars: {0})";

constexpr std::string_view kConfigVarsTemplate = R"(
  runtime_config_path: {0}
)";

void ValidateExampleCacheConfig(const formats::yaml::Value& static_config) {
  yaml_config::impl::Validate(
      yaml_config::YamlConfig(static_config["example-cache"], {}),
      ExampleCacheComponent::GetStaticConfigSchema());
}

}  // namespace

TEST_F(ComponentList, LruCacheComponentSample) {
  const auto temp_root = fs::blocking::TempDirectory::Create();
  const std::string dynamic_config_path =
      temp_root.GetPath() + "/dynamic_config.json";
  const std::string config_vars_path =
      temp_root.GetPath() + "/config_vars.json";

  const std::string static_config =
      fmt::format(kStaticConfigTemplate, config_vars_path);
  const std::string kConfigVariables =
      fmt::format(kConfigVarsTemplate, dynamic_config_path);

  /// [Sample lru cache component registration]
  auto component_list = components::MinimalComponentList();
  component_list.Append<ExampleCacheComponent>();
  /// [Sample lru cache component registration]
  component_list.Append<components::TestsuiteSupport>();

  fs::blocking::RewriteFileContents(dynamic_config_path, tests::kRuntimeConfig);
  fs::blocking::RewriteFileContents(config_vars_path, kConfigVariables);

  components::RunOnce(components::InMemoryConfig{static_config}, component_list,
                      "@null");
}

TEST(StaticConfigValidator, ValidConfig) {
  ValidateExampleCacheConfig(formats::yaml::FromString(
      std::string{kStaticConfigTemplate})["components_manager"]["components"]);
}

TEST(StaticConfigValidator, InvalidFieldName) {
  const std::string kInvalidStaticConfig = R"(
example-cache:
    size: 1
    ways: 1
    not_declared_property: 1
  )";
  UEXPECT_THROW_MSG(
      ValidateExampleCacheConfig(
          formats::yaml::FromString(kInvalidStaticConfig)),
      std::runtime_error,
      "Error while validating static config against schema. Field "
      "'example-cache.not_declared_property' is not declared in schema '/'");
}

TEST(StaticConfigValidator, InvalidFieldType) {
  const std::string kInvalidStaticConfig = R"(
example-cache:
    size: 1
    ways: abc # must be integer
)";
  UEXPECT_THROW_MSG(
      ValidateExampleCacheConfig(
          formats::yaml::FromString(kInvalidStaticConfig)),
      std::runtime_error,
      "Error while validating static config against schema. Value "
      "'abc' of field 'example-cache.ways' must be integer");
}

USERVER_NAMESPACE_END
