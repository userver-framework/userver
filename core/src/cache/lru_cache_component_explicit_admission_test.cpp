#include <userver/cache/lru_cache_component_base.hpp>

#include <atomic>
#include <chrono>
#include <cstddef>
#include <optional>
#include <stdexcept>
#include <string>
#include <string_view>
#include <utility>

#include <gtest/gtest.h>

#include <components/component_list_test.hpp>
#include <userver/cache/update_type.hpp>
#include <userver/components/component_base.hpp>
#include <userver/components/component_config.hpp>
#include <userver/components/component_context.hpp>
#include <userver/components/minimal_component_list.hpp>
#include <userver/components/run.hpp>
#include <userver/components/statistics_storage.hpp>
#include <userver/dynamic_config/updates_sink/component.hpp>
#include <userver/dynamic_config/updates_sink/find.hpp>
#include <userver/dynamic_config/value.hpp>
#include <userver/formats/json/serialize.hpp>
#include <userver/testsuite/cache_control.hpp>
#include <userver/testsuite/testsuite_support.hpp>
#include <userver/utils/fast_scope_guard.hpp>
#include <userver/utils/mock_now.hpp>
#include <userver/utils/statistics/testing.hpp>

USERVER_NAMESPACE_BEGIN

namespace {

using namespace std::chrono_literals;

// Explicit admission through GetOptionalNoUpdate/Put is a supported use of
// LruCacheComponent, including when DoGetByKey always throws. Component services
// must not require a loader, even when background-update is enabled.
class ExplicitAdmissionCache final : public cache::LruCacheComponent<std::string, int> {
public:
    static constexpr std::string_view kName = "explicit-admission-cache";

    using LruCacheComponent::GetCacheRaw;
    using LruCacheComponent::LruCacheComponent;

    ~ExplicitAdmissionCache() override { EXPECT_EQ(loader_calls_.load(), expected_loader_calls_); }

    void ExpectLoaderCalls(std::size_t count) { expected_loader_calls_ = count; }

private:
    int DoGetByKey(const std::string&) override {
        ++loader_calls_;
        throw std::logic_error("This cache only supports explicit admission");
    }

    std::atomic<std::size_t> loader_calls_{0};
    std::size_t expected_loader_calls_{0};
};

constexpr std::string_view kDriverName = "explicit-admission-driver";

void Publish(components::DynamicConfigUpdatesSinkBase& sink, std::string_view settings) {
    dynamic_config::DocsMap update;
    update.Set("USERVER_LRU_CACHES", formats::json::FromString(settings));
    sink.SetConfig(kDriverName, std::move(update));
}

utils::statistics::Snapshot GetMetrics(const components::ComponentContext& context) {
    return utils::statistics::Snapshot{
        context.FindComponent<components::StatisticsStorage>().GetStorage(),
        "cache",
        {{"cache_name", std::string{ExplicitAdmissionCache::kName}}},
    };
}

template <auto Check>
class ExplicitAdmissionDriver final : public components::ComponentBase {
public:
    static constexpr std::string_view kName = kDriverName;

    ExplicitAdmissionDriver(const components::ComponentConfig& config, const components::ComponentContext& context)
        : components::ComponentBase(config, context)
    {
        auto& sink = dynamic_config::FindUpdatesSink(config, context);
        Publish(sink, "{}");
        auto& component = context.FindComponent<ExplicitAdmissionCache>();

        utils::datetime::MockNowSet(std::chrono::system_clock::now());
        const utils::FastScopeGuard reset_time([]() noexcept { utils::datetime::MockNowUnset(); });
        Check(component, context, sink);

        EXPECT_EQ(GetMetrics(context).SingleMetric("background-updates").AsInt(), 0);
    }
};

class LruCacheExplicitAdmission : public ComponentList {
protected:
    template <auto Check>
    static void Run(std::string_view config_patch = "{}") {
        constexpr std::string_view kConfig = R"(
components_manager:
  components:
    dynamic-config:
      updates-enabled: true
    testsuite-support: {}
    explicit-admission-cache:
      size: 2
      ways: 1
      lifetime: 2s
    explicit-admission-driver: {}
)";
        const auto config = tests::MergeYaml(tests::MergeYaml(tests::kMinimalStaticConfig, kConfig), config_patch);
        components::RunOnce(
            components::InMemoryConfig{config},
            components::MinimalComponentList()
                .Append<components::TestsuiteSupport>()
                .Append<ExplicitAdmissionCache>()
                .Append<ExplicitAdmissionDriver<Check>>()
        );
    }
};

void CheckExplicitAdmission(ExplicitAdmissionCache& component, const components::ComponentContext&, components::DynamicConfigUpdatesSinkBase&) {
    const auto cache = component.GetCacheRaw();
    EXPECT_EQ(cache->GetOptionalNoUpdate("missing"), std::nullopt);
    EXPECT_EQ(cache->GetOptionalNoUpdate("missing"), std::nullopt);
    EXPECT_EQ(cache->GetSizeApproximate(), 0);

    cache->Put("key", 1);
    EXPECT_EQ(cache->GetOptionalNoUpdate("key"), 1);
    cache->Put("key", 2);
    EXPECT_EQ(cache->GetOptionalNoUpdate("key"), 2);
    EXPECT_EQ(cache->GetSizeApproximate(), 1);
}

void CheckLifetime(ExplicitAdmissionCache& component, const components::ComponentContext&, components::DynamicConfigUpdatesSinkBase&) {
    const auto cache = component.GetCacheRaw();
    cache->Put("key", 1);
    utils::datetime::MockSleep(1100ms);
    EXPECT_EQ(cache->GetOptionalNoUpdate("key"), 1);
    utils::datetime::MockSleep(1s);
    EXPECT_EQ(cache->GetOptionalNoUpdate("key"), std::nullopt);

    cache->Put("key", 2);
    utils::datetime::MockSleep(1100ms);
    EXPECT_EQ(cache->GetOptionalNoUpdate("key"), 2);
    utils::datetime::MockSleep(1s);
    EXPECT_EQ(cache->GetOptionalNoUpdate("key"), std::nullopt);
}

void CheckUnlimitedLifetime(ExplicitAdmissionCache& component, const components::ComponentContext&, components::DynamicConfigUpdatesSinkBase&) {
    const auto cache = component.GetCacheRaw();
    cache->Put("key", 1);
    utils::datetime::MockSleep(24h);
    EXPECT_EQ(cache->GetOptionalNoUpdate("key"), 1);
}

void CheckEviction(ExplicitAdmissionCache& component, const components::ComponentContext&, components::DynamicConfigUpdatesSinkBase&) {
    const auto cache = component.GetCacheRaw();
    cache->Put("first", 1);
    cache->Put("second", 2);
    EXPECT_EQ(cache->GetOptionalNoUpdate("first"), 1);
    cache->Put("third", 3);

    EXPECT_EQ(cache->GetSizeApproximate(), 2);
    EXPECT_EQ(cache->GetOptionalNoUpdate("second"), std::nullopt);
    EXPECT_EQ(cache->GetOptionalNoUpdate("first"), 1);
    EXPECT_EQ(cache->GetOptionalNoUpdate("third"), 3);
}

void CheckInvalidation(ExplicitAdmissionCache& component, const components::ComponentContext& context, components::DynamicConfigUpdatesSinkBase&) {
    const auto cache = component.GetCacheRaw();
    cache->Put("first", 1);
    cache->Put("second", 2);
    cache->InvalidateByKey("first");
    EXPECT_EQ(cache->GetOptionalNoUpdate("first"), std::nullopt);
    EXPECT_EQ(cache->GetOptionalNoUpdate("second"), 2);

    testsuite::FindCacheControl(context)
        .ResetCaches(cache::UpdateType::kFull, {std::string{ExplicitAdmissionCache::kName}}, {});
    EXPECT_EQ(cache->GetSizeApproximate(), 0);
    EXPECT_EQ(cache->GetOptionalNoUpdate("second"), std::nullopt);

    cache->Put("first", 3);
    EXPECT_EQ(cache->GetOptionalNoUpdate("first"), 3);
}

void CheckMetrics(ExplicitAdmissionCache& component, const components::ComponentContext& context, components::DynamicConfigUpdatesSinkBase&) {
    const auto cache = component.GetCacheRaw();
    EXPECT_EQ(cache->GetOptionalNoUpdate("key"), std::nullopt);
    cache->Put("key", 1);
    EXPECT_EQ(cache->GetOptionalNoUpdate("key"), 1);
    utils::datetime::MockSleep(3s);
    EXPECT_EQ(cache->GetOptionalNoUpdate("key"), std::nullopt);

    utils::datetime::MockSleep(std::chrono::ceil<
                               std::chrono::milliseconds>(cache->GetStatistics().recent_hits.GetEpochDuration()));
    const auto metrics = GetMetrics(context);
    EXPECT_EQ(metrics.SingleMetric("hits").AsInt(), 1);
    EXPECT_EQ(metrics.SingleMetric("misses").AsInt(), 2);
    EXPECT_EQ(metrics.SingleMetric("stale").AsInt(), 1);
    EXPECT_EQ(metrics.SingleMetric("hits.v2").AsRate().value, 1);
    EXPECT_EQ(metrics.SingleMetric("misses.v2").AsRate().value, 2);
    EXPECT_EQ(metrics.SingleMetric("stale.v2").AsRate().value, 1);
    EXPECT_EQ(metrics.SingleMetric("background-updates.v2").AsRate().value, 0);
    EXPECT_DOUBLE_EQ(metrics.SingleMetric("hit_ratio.1min").AsFloat(), 1.0 / 3.0);
    EXPECT_EQ(metrics.SingleMetric("current-documents-count").AsInt(), 1);

    cache->Invalidate();
    EXPECT_EQ(GetMetrics(context).SingleMetric("current-documents-count").AsInt(), 0);
}

constexpr std::string_view kDynamicSettings = R"(
{"explicit-admission-cache": {"size": 1, "lifetime-ms": 1000, "background-update": true}}
)";

void CheckDynamicSettings(
    ExplicitAdmissionCache& component,
    const components::ComponentContext&,
    components::DynamicConfigUpdatesSinkBase& sink
) {
    const auto cache = component.GetCacheRaw();
    cache->Put("old", 1);
    Publish(sink, kDynamicSettings);
    EXPECT_EQ(cache->GetMaxLifetime(), 1s);

    cache->Put("new", 2);
    EXPECT_EQ(cache->GetSizeApproximate(), 1);
    EXPECT_EQ(cache->GetOptionalNoUpdate("old"), std::nullopt);
    utils::datetime::MockSleep(600ms);
    EXPECT_EQ(cache->GetOptionalNoUpdate("new"), 2);
    utils::datetime::MockSleep(500ms);
    EXPECT_EQ(cache->GetOptionalNoUpdate("new"), std::nullopt);

    Publish(sink, "{}");
    EXPECT_EQ(cache->GetMaxLifetime(), 2s);
    cache->Invalidate();
    cache->Put("first", 1);
    cache->Put("second", 2);
    EXPECT_EQ(cache->GetSizeApproximate(), 2);
    utils::datetime::MockSleep(1100ms);
    EXPECT_EQ(cache->GetOptionalNoUpdate("first"), 1);
    EXPECT_EQ(cache->GetOptionalNoUpdate("second"), 2);
}

void CheckDisabledDynamicSettings(
    ExplicitAdmissionCache& component,
    const components::ComponentContext&,
    components::DynamicConfigUpdatesSinkBase& sink
) {
    const auto cache = component.GetCacheRaw();
    Publish(sink, kDynamicSettings);
    EXPECT_EQ(cache->GetMaxLifetime(), 2s);
    cache->Put("first", 1);
    cache->Put("second", 2);
    EXPECT_EQ(cache->GetSizeApproximate(), 2);
    utils::datetime::MockSleep(1100ms);
    EXPECT_EQ(cache->GetOptionalNoUpdate("first"), 1);
    EXPECT_EQ(cache->GetOptionalNoUpdate("second"), 2);
}

void CheckLoaderFailures(ExplicitAdmissionCache& component, const components::ComponentContext&, components::DynamicConfigUpdatesSinkBase&) {
    component.ExpectLoaderCalls(3);
    auto wrapper = component.GetCache();
    const auto cache = component.GetCacheRaw();
    EXPECT_THROW(wrapper.Get("key"), std::logic_error);
    EXPECT_THROW(wrapper.Get("key"), std::logic_error);
    EXPECT_EQ(cache->GetSizeApproximate(), 0);

    cache->Put("key", 1);
    EXPECT_EQ(wrapper.Get("key"), 1);
    utils::datetime::MockSleep(3s);
    EXPECT_THROW(wrapper.Get("key"), std::logic_error);
    EXPECT_EQ(cache->GetOptionalNoUpdate("key"), std::nullopt);

    cache->Put("key", 2);
    EXPECT_EQ(wrapper.Get("key"), 2);
}

TEST_F(LruCacheExplicitAdmission, MissesAndExplicitInsertion) { Run<CheckExplicitAdmission>(); }

TEST_F(LruCacheExplicitAdmission, OnlyInsertionRenewsLifetime) { Run<CheckLifetime>(); }

TEST_F(LruCacheExplicitAdmission, BackgroundUpdatesDoNotAffectManualAccess) {
    Run<CheckLifetime>(R"(
components_manager:
  components:
    explicit-admission-cache:
      background-update: true
)");
}

TEST_F(LruCacheExplicitAdmission, UnlimitedLifetime) {
    Run<CheckUnlimitedLifetime>(R"(
components_manager:
  components:
    explicit-admission-cache:
      lifetime: 0s
      background-update: true
)");
}

TEST_F(LruCacheExplicitAdmission, EvictsLeastRecentlyUsedEntry) { Run<CheckEviction>(); }

TEST_F(LruCacheExplicitAdmission, InvalidationAndTestsuiteReset) { Run<CheckInvalidation>(); }

TEST_F(LruCacheExplicitAdmission, ExportsMetricsWithoutLoader) { Run<CheckMetrics>(); }

TEST_F(LruCacheExplicitAdmission, AppliesDynamicSettingsAndRestoresStaticSettings) { Run<CheckDynamicSettings>(); }

TEST_F(LruCacheExplicitAdmission, CanDisableDynamicSettings) {
    Run<CheckDisabledDynamicSettings>(R"(
components_manager:
  components:
    explicit-admission-cache:
      config-settings: false
)");
}

TEST_F(LruCacheExplicitAdmission, LoaderFailuresAreNotCachedAndDoNotPreventExplicitInsertion) {
    Run<CheckLoaderFailures>();
}

}  // namespace

USERVER_NAMESPACE_END
