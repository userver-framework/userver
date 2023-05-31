#include <userver/dynamic_config/updates_sink/component.hpp>

#include <string>
#include <string_view>

#include <fmt/format.h>
#include <gmock/gmock.h>

#include <userver/components/component.hpp>
#include <userver/components/minimal_component_list.hpp>
#include <userver/components/run.hpp>
#include <userver/dynamic_config/storage/component.hpp>
#include <userver/dynamic_config/updates_sink/find.hpp>
#include <userver/dynamic_config/value.hpp>
#include <userver/formats/json/value_builder.hpp>
#include <userver/fs/blocking/temp_directory.hpp>
#include <userver/fs/blocking/write.hpp>
#include <userver/yaml_config/merge_schemas.hpp>

#include <components/component_list_test.hpp>
#include <userver/utest/utest.hpp>

USERVER_NAMESPACE_BEGIN

namespace {

constexpr std::string_view kConfigVarsTemplate = R"(
  runtime_config_path: {0}
  sink1_updates_sink: {1}
)";

constexpr std::string_view kStaticConfig = R"(
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
    logging:
      fs-task-processor: main-task-processor
      loggers:
        default:
          file_path: '@null'
    tracer:
        service-name: config-service
    statistics-storage:
      # Nothing
    dynamic-config:
      fs-cache-path: $runtime_config_path
      fs-task-processor: main-task-processor
# /// [Sample dynamic config updates sink component]
# yaml
    dynamic-config-test-updates-sink1:
      updates-sink: $sink1_updates_sink
    dynamic-config-test-updates-sink2:
      # Nothing
# /// [Sample dynamic config updates sink component]
# /// [Sample dynamic config fallback component]
# yaml
    dynamic-config-fallbacks:
      fallback-path: $runtime_config_path
      updates-sink: dynamic-config-test-updates-sink1
# /// [Sample dynamic config fallback component]
# /// [Verifier of the observed updates sink chain]
# yaml
    updates-sink-chain-verifier:
      # Nothing
# /// [Verifier of the observed updates sink chain]
config_vars: )";

constexpr std::string_view kUpdatesSinkChainConfigName =
    "DYNAMIC_CONFIG_UPDATES_SINK_CHAIN";

std::string ParseUpdatesSinkChain(const dynamic_config::DocsMap& config) {
  return config.Get(std::string(kUpdatesSinkChainConfigName)).As<std::string>();
}

constexpr dynamic_config::Key<ParseUpdatesSinkChain> kUpdatesSinkChain{};
std::string expected_updates_sink_chain;

class TestUpdatesSink final : public components::DynamicConfigUpdatesSinkBase {
 public:
  TestUpdatesSink(const components::ComponentConfig&,
                  const components::ComponentContext&);

  void SetConfig(std::string_view updater,
                 dynamic_config::DocsMap&& config) final;

  void NotifyLoadingFailed(std::string_view updater,
                           std::string_view error) final;

  static yaml_config::Schema GetStaticConfigSchema();

 private:
  std::string name_;
  components::DynamicConfigUpdatesSinkBase& next_sink_;
};

TestUpdatesSink::TestUpdatesSink(const components::ComponentConfig& config,
                                 const components::ComponentContext& context)
    : components::DynamicConfigUpdatesSinkBase(config, context),
      name_(config.Name()),
      next_sink_(dynamic_config::FindUpdatesSink(config, context)) {}

void TestUpdatesSink::SetConfig(std::string_view updater,
                                dynamic_config::DocsMap&& config) {
  auto sinks_chain =
      config.Get(std::string(kUpdatesSinkChainConfigName)).As<std::string>();

  if (!sinks_chain.empty()) {
    sinks_chain.append(" ");
  }

  sinks_chain.append(updater);

  if (dynamic_cast<components::DynamicConfig*>(&next_sink_)) {
    sinks_chain.append(fmt::format(" {} {}", name_, "dynamic-config"));
  }

  config.Set(std::string(kUpdatesSinkChainConfigName),
             formats::json::ValueBuilder(sinks_chain).ExtractValue());
  next_sink_.SetConfig(name_, std::move(config));
}

void TestUpdatesSink::NotifyLoadingFailed(std::string_view updater,
                                          std::string_view error) {
  next_sink_.NotifyLoadingFailed(updater, error);
}

yaml_config::Schema TestUpdatesSink::GetStaticConfigSchema() {
  return yaml_config::MergeSchemas<LoggableComponentBase>(R"(
type: object
description: Test updates sink component.
additionalProperties: false
properties:
    updates-sink:
        type: string
        description: components::DynamicConfigUpdatesSinkBase descendant to be used for storing fallback config
        defaultDescription: empty string, treated as if `dynamic-config` was specified
)");
}

class ChainVerifier final : public components::LoggableComponentBase {
 public:
  static constexpr std::string_view kName = "updates-sink-chain-verifier";

  ChainVerifier(const components::ComponentConfig& config,
                const components::ComponentContext& context)
      : components::LoggableComponentBase(config, context),
        source_(
            context.FindComponent<components::DynamicConfig>().GetSource()) {}

  static yaml_config::Schema GetStaticConfigSchema() {
    return yaml_config::MergeSchemas<components::LoggableComponentBase>(R"(
type: object
description: Component that verifies chain of 'SetConfig' calls
additionalProperties: false
properties: {}
)");
  }

  void OnAllComponentsLoaded() final {
    auto snapshot = source_.GetSnapshot();
    EXPECT_EQ(expected_updates_sink_chain, snapshot[kUpdatesSinkChain]);
  }

 private:
  dynamic_config::Source source_;
};

components::ComponentList MakeComponentList() {
  return components::MinimalComponentList()
      .Append<TestUpdatesSink>("dynamic-config-test-updates-sink1")
      .Append<TestUpdatesSink>("dynamic-config-test-updates-sink2")
      .Append<ChainVerifier>();
}

}  // namespace

TEST_F(ComponentList, DynamicConfigUpdatesSink) {
  expected_updates_sink_chain =
      "dynamic-config-fallbacks dynamic-config-test-updates-sink1 "
      "dynamic-config-test-updates-sink2 dynamic-config";
  const auto temp_root = fs::blocking::TempDirectory::Create();
  const std::string runtime_config_path =
      temp_root.GetPath() + "/runtime_config.json";
  const std::string config_vars_path =
      temp_root.GetPath() + "/config_vars.json";
  const std::string static_config =
      std::string{kStaticConfig} + config_vars_path + '\n';

  fs::blocking::RewriteFileContents(runtime_config_path, tests::kRuntimeConfig);
  fs::blocking::RewriteFileContents(
      config_vars_path, fmt::format(kConfigVarsTemplate, runtime_config_path,
                                    "dynamic-config-test-updates-sink2"));

  components::RunOnce(components::InMemoryConfig{static_config},
                      MakeComponentList(), "@null");
}

TEST_F(ComponentList, DynamicConfigUpdatesSinkUsedByMultipleSources) {
  const auto temp_root = fs::blocking::TempDirectory::Create();
  const std::string runtime_config_path =
      temp_root.GetPath() + "/runtime_config.json";
  const std::string config_vars_path =
      temp_root.GetPath() + "/config_vars.json";
  const std::string static_config =
      std::string{kStaticConfig} + config_vars_path + '\n';

  fs::blocking::RewriteFileContents(runtime_config_path, tests::kRuntimeConfig);
  fs::blocking::RewriteFileContents(
      config_vars_path,
      fmt::format(kConfigVarsTemplate, runtime_config_path, "dynamic-config"));

  try {
    components::RunOnce(components::InMemoryConfig{static_config},
                        MakeComponentList(), "@null");
    ADD_FAILURE() << "should throw 'std::runtime_error' exception";
  } catch (const std::runtime_error& e) {
    const std::string text = e.what();
    EXPECT_THAT(text, testing::HasSubstr("dynamic-config"));
    EXPECT_THAT(text, testing::HasSubstr("dynamic-config-test-updates-sink1"));
    EXPECT_THAT(text, testing::HasSubstr("dynamic-config-test-updates-sink2"));
  } catch (...) {
    ADD_FAILURE() << "expected 'std::runtime_error' but exception of another "
                     "type is caught";
  }
}

USERVER_NAMESPACE_END
