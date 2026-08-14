#include <userver/dynamic_config/storage/component.hpp>

#include <string_view>

#include <userver/components/component.hpp>
#include <userver/components/minimal_component_list.hpp>
#include <userver/components/run.hpp>
#include <userver/dynamic_config/snapshot.hpp>
#include <userver/dynamic_config/updates_sink/find.hpp>
#include <userver/dynamic_config/value.hpp>
#include <userver/formats/json/value_builder.hpp>
#include <userver/yaml_config/merge_schemas.hpp>

#include <components/component_list_test.hpp>
#include <userver/utest/utest.hpp>

USERVER_NAMESPACE_BEGIN

namespace {

// A dedicated key (unrelated to any real production config), used only to
// verify that `dynamic-config.defaults` overrides are applied consistently by
// both `GetSource()` and `GetDefaultsAsConstantSource()`.
const dynamic_config::Key<int> kTestIntConfig{"USERVER_TEST_LIGHT_CLIENT_INT_CONFIG", 0};

// Simulates a "light" component with a blocking-dependency-avoiding use of
// `GetDefaultsAsConstantSource()` in its constructor (before the real config
// updater component has been constructed and could have unblocked
// `GetSource()`), and later observes the real, updated config once it is
// safe to do so (`OnAllComponentsLoaded()`, by which point the updater
// component below has already run and pushed an update).
class ConstantSourceVerifier final : public components::ComponentBase {
public:
    static constexpr std::string_view kName = "constant-source-verifier";

    ConstantSourceVerifier(const components::ComponentConfig& config, const components::ComponentContext& context)
        : components::ComponentBase(config, context),
          dynamic_config_(context.FindComponent<components::DynamicConfig>())
    {
        // Must be safe to call here: the config updater component below has not
        // been constructed yet, so `GetSource()` would block forever if used
        // instead. The constant source, in contrast, only ever reflects
        // `dynamic-config.defaults`.
        const auto constant_snapshot = dynamic_config_.GetDefaultsAsConstantSource().GetSnapshot();
        EXPECT_EQ(constant_snapshot[kTestIntConfig], 42);
    }

    static yaml_config::Schema GetStaticConfigSchema() {
        return yaml_config::MergeSchemas<components::ComponentBase>(R"(
type: object
description: Test component verifying GetDefaultsAsConstantSource().
additionalProperties: false
properties: {}
)");
    }

    void OnAllComponentsLoaded() final {
        // By this phase, ConfigUpdaterDependingOnVerifier (which depends on this
        // component and is therefore constructed after it) has already pushed a
        // real config update, so GetSource() is safe to use and reflects it.
        const auto real_snapshot = dynamic_config_.GetSource().GetSnapshot();
        EXPECT_EQ(real_snapshot[kTestIntConfig], 100);

        // The constant source must still reflect the original default,
        // regardless of the real update pushed above.
        const auto constant_snapshot = dynamic_config_.GetDefaultsAsConstantSource().GetSnapshot();
        EXPECT_EQ(constant_snapshot[kTestIntConfig], 42);
    }

private:
    components::DynamicConfig& dynamic_config_;
};

// Depends on ConstantSourceVerifier to guarantee construction order: by the
// time this component's constructor runs, ConstantSourceVerifier has already
// exercised GetDefaultsAsConstantSource(). Pushes a real config update, which
// ConstantSourceVerifier::OnAllComponentsLoaded() then observes via
// GetSource().
class ConfigUpdaterDependingOnVerifier final : public components::ComponentBase {
public:
    static constexpr std::string_view kName = "config-updater-depending-on-verifier";

    ConfigUpdaterDependingOnVerifier(
        const components::ComponentConfig& config,
        const components::ComponentContext& context
    )
        : components::ComponentBase(config, context) {
        context.FindComponent<ConstantSourceVerifier>();

        dynamic_config::DocsMap update;
        update.Set(std::string{kTestIntConfig.GetName()}, formats::json::ValueBuilder{100}.ExtractValue());
        dynamic_config::FindUpdatesSink(config, context).SetConfig(kName, std::move(update));
    }

    static yaml_config::Schema GetStaticConfigSchema() {
        return yaml_config::MergeSchemas<components::ComponentBase>(R"(
type: object
description: Test component pushing a real dynamic config update.
additionalProperties: false
properties: {}
)");
    }
};

constexpr std::string_view kStaticConfig = R"(
components_manager:
  coro_pool:
    initial_size: 5
    max_size: 50
    stack_usage_monitor_enabled: false
  default_task_processor: main-task-processor
  fs_task_processor: main-task-processor
  event_thread_pool:
    threads: 1
  task_processors:
    main-task-processor:
      worker_threads: 1
  components:
    logging:
      fs-task-processor: main-task-processor
      loggers:
        default:
          file_path: '@null'
    dynamic-config:
      updates-enabled: true
      defaults:
          USERVER_TEST_LIGHT_CLIENT_INT_CONFIG: 42
    constant-source-verifier:
      # Nothing
    config-updater-depending-on-verifier:
      # Nothing
)";

}  // namespace

TEST_F(ComponentList, DynamicConfigGetDefaultsAsConstantSource) {
    const auto component_list =
        components::MinimalComponentList().Append<ConstantSourceVerifier>().Append<ConfigUpdaterDependingOnVerifier>();
    components::RunOnce(components::InMemoryConfig{std::string{kStaticConfig}}, component_list);
}

USERVER_NAMESPACE_END
