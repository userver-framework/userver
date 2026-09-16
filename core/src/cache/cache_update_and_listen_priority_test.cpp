#include <gmock/gmock.h>

#include <string>
#include <vector>

#include <fmt/format.h>

#include <components/component_list_test.hpp>
#include <userver/cache/caching_component_base.hpp>
#include <userver/components/minimal_component_list.hpp>
#include <userver/components/run.hpp>
#include <userver/dynamic_config/storage_mock.hpp>
#include <userver/testsuite/testsuite_support.hpp>
#include <userver/utest/utest.hpp>

USERVER_NAMESPACE_BEGIN

namespace {

const dynamic_config::Key kTestIntConfig{dynamic_config::ConstantConfig{}, 0};

std::vector<std::string> trace;

const auto kStaticConfig = tests::MergeYaml(tests::kMinimalStaticConfig, R"(
components_manager:
    components:
        testsuite-support:
        listening-cache:
            update-types: only-full
            update-interval: 1h
)");

class CacheListeningToConfig final : public components::CachingComponentBase<int> {
public:
    static constexpr std::string_view kName = "listening-cache";

    CacheListeningToConfig(const components::ComponentConfig& config, const components::ComponentContext& context)
        : CachingComponentBase(config, context),
          storage_({{kTestIntConfig, 0}})
    {
        storage_.GetSource()
            .UpdateAndListen(context.Scopes(), this, kName, &CacheListeningToConfig::OnConfigUpdate, kTestIntConfig);
        trace.push_back("before Extend");
        storage_.Extend({{kTestIntConfig, 7}});
        trace.push_back("after Extend");
    }

private:
    void OnConfigUpdate(const dynamic_config::Snapshot& config) {
        applied_value_ = config[kTestIntConfig];
        trace.push_back(fmt::format("OnConfigUpdate, config={}", applied_value_));
    }

    void Update(
        cache::UpdateType /*type*/,
        const std::chrono::system_clock::time_point& /*last_update*/,
        const std::chrono::system_clock::time_point& /*now*/,
        cache::UpdateStatisticsScope& stats_scope
    ) override {
        trace.push_back(fmt::format("Update, config={}", applied_value_));
        Emplace(applied_value_);
        stats_scope.Finish(1);
    }

    dynamic_config::StorageMock storage_;
    int applied_value_{0};
};

class CacheUpdateAndListenPriorityTest : public ComponentList {};

}  // namespace

TEST_F(CacheUpdateAndListenPriorityTest, ActualizesBeforeFirstCacheUpdate)
{
    trace = {};

    components::RunOnce(
        components::InMemoryConfig{kStaticConfig},
        components::MinimalComponentList().Append<components::TestsuiteSupport>().Append<CacheListeningToConfig>()
    );

    EXPECT_THAT(
        trace,
        ::testing::AnyOf(
            // Release: no extra callback on unsubscribe.
            ::testing::ElementsAre(
                "OnConfigUpdate, config=0",
                "before Extend",
                "after Extend",
                "OnConfigUpdate, config=7",
                "Update, config=7"
            ),
            // Debug: AsyncEventChannel fakes a last Snapshot to check UB.
            ::testing::ElementsAre(
                "OnConfigUpdate, config=0",
                "before Extend",
                "after Extend",
                "OnConfigUpdate, config=7",
                "Update, config=7",
                "OnConfigUpdate, config=7"
            )
        )
    );
}

USERVER_NAMESPACE_END
