#include <cache/lru_cache_component_base_test.hpp>

#include <chrono>
#include <cstddef>
#include <string>
#include <string_view>
#include <utility>

#include <components/component_list_test.hpp>
#include <userver/components/component_config.hpp>
#include <userver/components/component_context.hpp>
#include <userver/components/minimal_component_list.hpp>
#include <userver/components/run.hpp>
#include <userver/components/statistics_storage.hpp>
#include <userver/dynamic_config/storage/component.hpp>
#include <userver/dynamic_config/updates_sink/find.hpp>
#include <userver/dynamic_config/value.hpp>
#include <userver/formats/json/serialize.hpp>
#include <userver/fs/blocking/temp_directory.hpp>
#include <userver/fs/blocking/write.hpp>
#include <userver/testsuite/testsuite_support.hpp>
#include <userver/utils/statistics/json.hpp>
#include <userver/yaml_config/merge_schemas.hpp>

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

class ConfigurableLruCache final : public cache::LruCacheComponent<std::string, std::size_t> {
public:
    static constexpr std::string_view kName = "configurable-lru-cache";

    using LruCacheComponent::GetCacheRaw;

    ConfigurableLruCache(const components::ComponentConfig& config, const components::ComponentContext& context)
        : LruCacheComponent(config, PrepareContext(config, context))
    {}

    static yaml_config::Schema GetStaticConfigSchema() {
        return yaml_config::MergeSchemas<LruCacheComponent>(R"(
type: object
description: LRU cache for testing dynamic configuration
additionalProperties: false
properties:
    require-config-before-construction:
        type: boolean
        description: Ensure dynamic config is loaded before constructing the base cache
        default: false
)");
    }

private:
    static const components::ComponentContext& PrepareContext(
        const components::ComponentConfig& config,
        const components::ComponentContext& context
    ) {
        if (config["require-config-before-construction"].As<bool>(false)) {
            context.FindComponent<components::DynamicConfig>().GetSource();
        }
        return context;
    }

    std::size_t DoGetByKey(const std::string& key) override { return key.size(); }
};

void CheckCacheSettings(ConfigurableLruCache& component, std::size_t size, std::chrono::milliseconds lifetime) {
    const auto cache = component.GetCacheRaw();
    EXPECT_EQ(cache->GetMaxLifetime(), lifetime);
    cache->Invalidate();
    for (std::size_t index = 0; index <= size; ++index) {
        cache->Put(std::to_string(index), index);
    }
    EXPECT_EQ(cache->GetSizeApproximate(), size);
}

class LruConfigUpdater final : public components::ComponentBase {
public:
    static constexpr std::string_view kName = "lru-config-updater";

    LruConfigUpdater(const components::ComponentConfig& config, const components::ComponentContext& context)
        : components::ComponentBase(config, context) {
        using namespace std::chrono_literals;
        auto& sink = dynamic_config::FindUpdatesSink(config, context);
        const bool config_before_cache = config["config-before-cache"].As<bool>(false);
        const bool dynamic_settings = config["dynamic-settings"].As<bool>(true);
        if (config_before_cache) {
            Publish(sink, R"({"configurable-lru-cache": {"size": 3, "lifetime-ms": 2000}})");
        }

        auto& cache = context.FindComponent<ConfigurableLruCache>();
        if (config_before_cache && dynamic_settings) {
            CheckCacheSettings(cache, 3, 2s);
        } else {
            CheckCacheSettings(cache, 1, 1s);
        }

        Publish(sink, R"({"configurable-lru-cache": {"size": 3, "lifetime-ms": 2000}})");
        CheckCacheSettings(cache, dynamic_settings ? 3 : 1, dynamic_settings ? 2s : 1s);
        Publish(sink, R"({"configurable-lru-cache": {"size": 2, "lifetime-ms": 3000}})");
        CheckCacheSettings(cache, dynamic_settings ? 2 : 1, dynamic_settings ? 3s : 1s);
        Publish(sink, "{}");
        CheckCacheSettings(cache, 1, 1s);

        const auto& statistics = context.FindComponent<components::StatisticsStorage>().GetStorage();
        const auto metrics = formats::json::FromString(utils::statistics::ToJsonFormat(statistics));
        EXPECT_EQ(metrics["cache.current-documents-count"][0]["value"].As<std::size_t>(), 1);
    }

    static yaml_config::Schema GetStaticConfigSchema() {
        return yaml_config::MergeSchemas<components::ComponentBase>(R"(
type: object
description: Publish dynamic settings and verify the LRU cache
additionalProperties: false
properties:
    config-before-cache:
        type: boolean
        description: Publish the first config before constructing the cache
        default: false
    dynamic-settings:
        type: boolean
        description: Expect the cache to apply dynamic settings
        default: true
)");
    }

private:
    static void Publish(components::DynamicConfigUpdatesSinkBase& sink, std::string_view settings) {
        dynamic_config::DocsMap update;
        update.Set("USERVER_LRU_CACHES", formats::json::FromString(settings));
        sink.SetConfig(kName, std::move(update));
    }
};

constexpr std::string_view kDynamicSettingsConfig = R"(
components_manager:
  components:
    dynamic-config:
      updates-enabled: true
    testsuite-support: {}
    configurable-lru-cache:
      size: 1
      ways: 1
      lifetime: 1s
    lru-config-updater: {}
)";

void RunDynamicSettingsTest(std::string_view config_patch) {
    const auto
        config = tests::MergeYaml(tests::MergeYaml(tests::kMinimalStaticConfig, kDynamicSettingsConfig), config_patch);
    const auto components =
        components::MinimalComponentList()
            .Append<ConfigurableLruCache>()
            .Append<LruConfigUpdater>()
            .Append<components::TestsuiteSupport>();
    components::RunOnce(components::InMemoryConfig{config}, components);
}

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

TEST_F(ComponentList, LruCacheDoesNotWaitForFirstDynamicConfig) {
    RunDynamicSettingsTest(R"(
components_manager:
  components:
    configurable-lru-cache:
      wait-for-dynamic-configs: false
)");
}

TEST_F(ComponentList, LruCacheUsesAlreadyLoadedConfigWithoutWaiting) {
    RunDynamicSettingsTest(R"(
components_manager:
  components:
    configurable-lru-cache:
      wait-for-dynamic-configs: false
      require-config-before-construction: true
    lru-config-updater:
      config-before-cache: true
)");
}

TEST_F(ComponentList, LruCacheUsesDynamicConfigByDefault) {
    RunDynamicSettingsTest(R"(
components_manager:
  components:
    lru-config-updater:
      config-before-cache: true
)");
}

TEST_F(ComponentList, LruCacheIgnoresDynamicConfigWhenSettingsDisabled) {
    RunDynamicSettingsTest(R"(
components_manager:
  components:
    configurable-lru-cache:
      config-settings: false
    lru-config-updater:
      dynamic-settings: false
)");
}

USERVER_NAMESPACE_END
