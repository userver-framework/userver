#include <userver/utest/utest.hpp>

#include <chrono>
#include <optional>
#include <string>

#include <boost/filesystem/operations.hpp>

#include <userver/cache/cache_config.hpp>
#include <userver/cache/cache_update_trait.hpp>
#include <userver/components/component_base.hpp>
#include <userver/concurrent/variable.hpp>
#include <userver/dump/common.hpp>
#include <userver/dump/unsafe.hpp>
#include <userver/formats/yaml/serialize.hpp>
#include <userver/fs/blocking/temp_directory.hpp>
#include <userver/fs/blocking/write.hpp>
#include <userver/testsuite/cache_control.hpp>
#include <userver/yaml_config/yaml_config.hpp>

#include <userver/alerts/component.hpp>
#include <userver/components/component_list.hpp>
#include <userver/components/run.hpp>
#include <userver/components/statistics_storage.hpp>
#include <userver/logging/component.hpp>
#include <userver/os_signals/component.hpp>
#include <userver/testsuite/testsuite_support.hpp>
#include <userver/tracing/component.hpp>

#include <cache/internal_helpers_test.hpp>
#include <components/component_list_test.hpp>
#include <dump/internal_helpers_test.hpp>

USERVER_NAMESPACE_BEGIN

namespace {

const std::string kCacheName = "test_cache";
const std::string kCacheNameAlternative = "test_cache_alternative";
const std::string kDumpToRead = "2015-03-22T090000.000000Z-v0";
constexpr std::size_t kDummyDocumentsCount = 42;

class FakeCache final : public cache::CacheMockBase {
public:
    FakeCache(std::string_view name, const yaml_config::YamlConfig& config, cache::MockEnvironment& environment)
        : CacheMockBase(name, config, environment) {
        StartPeriodicUpdates(cache::CacheUpdateTrait::Flag::kNoFirstUpdate);
    }

    ~FakeCache() final { StopPeriodicUpdates(); }

    const std::string& Get() const { return value_; }
    size_t UpdatesCount() const { return updates_count_; }
    cache::UpdateType LastUpdateType() const { return last_update_type_; }

private:
    void Update(
        cache::UpdateType type,
        const std::chrono::system_clock::time_point&,
        const std::chrono::system_clock::time_point&,
        cache::UpdateStatisticsScope& stats_scope
    ) override {
        ++updates_count_;
        last_update_type_ = type;
        value_ = "foo";
        OnCacheModified();
        stats_scope.Finish(kDummyDocumentsCount);
    }

    void GetAndWrite(dump::Writer& writer) const override { dump::WriteStringViewUnsafe(writer, value_); }

    void ReadAndSet(dump::Reader& reader) override { value_ = dump::ReadEntire(reader); }

    std::string value_;
    size_t updates_count_{0};
    cache::UpdateType last_update_type_{cache::UpdateType::kIncremental};
};

const std::string kConfigContents = R"(
update-interval: 1s
update-jitter: 1s
full-update-interval: 1s
additional-cleanup-interval: 1s
dump:
    enable: true
    world-readable: false
    format-version: 0
    first-update-mode: required
    first-update-type: full
)";

}  // namespace

UTEST(CacheControl, Smoke) {
    const yaml_config::YamlConfig config{formats::yaml::FromString(kConfigContents), {}};
    cache::MockEnvironment env;

    FakeCache test_cache(kCacheName, config, env);

    FakeCache test_cache_alternative(kCacheNameAlternative, config, env);

    // Periodic updates are disabled, so a synchronous update will be performed
    EXPECT_EQ(1, test_cache.UpdatesCount());

    env.cache_control.ResetCaches(
        cache::UpdateType::kFull,
        {kCacheName},
        /*force_incremental_names=*/{}
    );
    EXPECT_EQ(2, test_cache.UpdatesCount());
    EXPECT_EQ(cache::UpdateType::kFull, test_cache.LastUpdateType());

    env.cache_control.ResetCaches(
        cache::UpdateType::kIncremental,
        {kCacheNameAlternative},
        /*force_incremental_names=*/{}
    );
    EXPECT_EQ(2, test_cache.UpdatesCount());
    EXPECT_EQ(cache::UpdateType::kFull, test_cache.LastUpdateType());

    env.cache_control.ResetCaches(
        cache::UpdateType::kIncremental,
        {},
        /*force_incremental_names=*/{}
    );
    EXPECT_EQ(2, test_cache.UpdatesCount());
    EXPECT_EQ(cache::UpdateType::kFull, test_cache.LastUpdateType());

    env.cache_control.ResetAllCaches(
        cache::UpdateType::kIncremental,
        /*force_incremental_names=*/{},
        /*exclude_names=*/{}
    );
    EXPECT_EQ(3, test_cache.UpdatesCount());
    EXPECT_EQ(cache::UpdateType::kIncremental, test_cache.LastUpdateType());

    EXPECT_EQ(test_cache.Get(), "foo");

    boost::filesystem::remove_all(env.dump_root.GetPath());
    dump::CreateDumps({kDumpToRead}, env.dump_root, kCacheName);
    env.dump_control.ReadCacheDumps({kCacheName});
    EXPECT_EQ(test_cache.Get(), kDumpToRead);

    boost::filesystem::remove_all(env.dump_root.GetPath());
    env.cache_control.ResetCaches(
        cache::UpdateType::kFull,
        {kCacheName},
        /*force_incremental_names=*/{}
    );
    env.dump_control.WriteCacheDumps({kCacheName});
    EXPECT_EQ(dump::FilenamesInDirectory(env.dump_root, kCacheName).size(), 1);
}

UTEST_DEATH(CacheControlDeathTest, MissingCache) {
    const yaml_config::YamlConfig config{formats::yaml::FromString(kConfigContents), {}};
    cache::MockEnvironment env;
    FakeCache test_cache(kCacheName, config, env);

    EXPECT_UINVARIANT_FAILURE(env.dump_control.WriteCacheDumps({"missing"}));
    EXPECT_UINVARIANT_FAILURE(env.dump_control.ReadCacheDumps({"missing"}));
}

namespace {

/// [sample]
class MyCache final : public components::ComponentBase {
public:
    static constexpr std::string_view kName = "my-component-default-name";

    MyCache(const components::ComponentConfig& config, const components::ComponentContext& context)
        : components::ComponentBase(config, context) {
        {
            auto cache = cached_token_.Lock();
            *cache = FetchToken();
        }
        // ...

        // reset_registration_ must be set at the end of the constructor.
        reset_registration_ = testsuite::RegisterCache(config, context, this, &MyCache::ResetCache);
    }

    std::string GetToken() {
        auto cache = cached_token_.Lock();
        if (auto& opt_token = *cache; opt_token) return *opt_token;
        auto new_token = FetchToken();
        *cache = new_token;
        return new_token;
    }

    void ReportServiceRejectedToken() { ResetCache(); }

private:
    std::string FetchToken() const;

    void ResetCache() {
        auto cache = cached_token_.Lock();
        cache->reset();
    }

    concurrent::Variable<std::optional<std::string>> cached_token_;

    // Subscriptions must be the last fields.
    testsuite::CacheResetRegistration reset_registration_;
};
/// [sample]

std::string MyCache::FetchToken() const { return "foo"; }

std::atomic<std::size_t> expected_resets_count{0};

void AssertConcurrentResets();

class Component1 final : public components::ComponentBase {
public:
    static constexpr std::string_view kName = "component-1";

    Component1(const components::ComponentConfig& config, const components::ComponentContext& context)
        : components::ComponentBase(config, context) {
        reset_registration_ = testsuite::RegisterCache(config, context, this, &Component1::ResetCache);
    }

    void ResetCache() {
        ++Component1::resets_count;
        AssertConcurrentResets();
    }

    static inline std::atomic<std::size_t> resets_count{0};

private:
    // Subscriptions must be the last fields.
    testsuite::CacheResetRegistration reset_registration_;
};

class Component1a final : public components::ComponentBase {
public:
    static constexpr std::string_view kName = "component-1a";

    Component1a(const components::ComponentConfig& config, const components::ComponentContext& context)
        : components::ComponentBase(config, context) {
        reset_registration_ = testsuite::RegisterCache(config, context, this, &Component1a::ResetCache);
    }

    void ResetCache() {
        ++Component1a::resets_count;
        AssertConcurrentResets();
    }

    static inline std::atomic<std::size_t> resets_count{0};

private:
    // Subscriptions must be the last fields.
    testsuite::CacheResetRegistration reset_registration_;
};

class Component1b final : public components::ComponentBase {
public:
    static constexpr std::string_view kName = "component-1b";

    Component1b(const components::ComponentConfig& config, const components::ComponentContext& context)
        : components::ComponentBase(config, context) {
        reset_registration_ = testsuite::RegisterCache(config, context, this, &Component1b::ResetCache);
    }

    void ResetCache() {
        ++Component1b::resets_count;
        AssertConcurrentResets();
    }

    static inline std::atomic<std::size_t> resets_count{0};

private:
    // Subscriptions must be the last fields.
    testsuite::CacheResetRegistration reset_registration_;
};

class Component1c final : public components::ComponentBase {
public:
    static constexpr std::string_view kName = "component-1c";

    Component1c(const components::ComponentConfig& config, const components::ComponentContext& context)
        : components::ComponentBase(config, context) {
        reset_registration_ = testsuite::RegisterCache(config, context, this, &Component1c::ResetCache);
    }

    void ResetCache() {
        ++Component1c::resets_count;
        AssertConcurrentResets();
    }

    static inline std::atomic<std::size_t> resets_count{0};

private:
    // Subscriptions must be the last fields.
    testsuite::CacheResetRegistration reset_registration_;
};

class Component2 final : public components::ComponentBase {
public:
    static constexpr std::string_view kName = "component-2";

    Component2(const components::ComponentConfig& config, const components::ComponentContext& context)
        : components::ComponentBase(config, context) {
        context.FindComponent<Component1>();
        reset_registration_ = testsuite::RegisterCache(config, context, this, &Component2::ResetCache);
    }

    void ResetCache() {
        AssertConcurrentResets();
        ASSERT_EQ(++Component2::resets_count, expected_resets_count);
    }

    static inline std::atomic<std::size_t> resets_count{0};

private:
    // Subscriptions must be the last fields.
    testsuite::CacheResetRegistration reset_registration_;
};

class ComponentNotLoaded final : public components::ComponentBase {
public:
    static constexpr std::string_view kName = "component-not-loaded";

    ComponentNotLoaded(const components::ComponentConfig& config, const components::ComponentContext& context)
        : components::ComponentBase(config, context) {
        context.FindComponent<Component1>();
        reset_registration_ = testsuite::RegisterCache(config, context, this, &ComponentNotLoaded::ResetCache);
    }

    void ResetCache() { UASSERT(false); }

private:
    // Subscriptions must be the last fields.
    testsuite::CacheResetRegistration reset_registration_;
};

class Component3 final : public components::ComponentBase {
public:
    static constexpr std::string_view kName = "component-3";

    Component3(const components::ComponentConfig& config, const components::ComponentContext& context)
        : components::ComponentBase(config, context), cc_{testsuite::FindCacheControl(context)} {
        context.FindComponentOptional<Component2>();
        context.FindComponentOptional<ComponentNotLoaded>();

        context.FindComponent<Component1a>();
        context.FindComponent<Component1b>();
        context.FindComponent<Component1c>();

        reset_registration_ = testsuite::RegisterCache(config, context, this, &Component3::ResetCache);
    }

    void ResetCache() {
        AssertConcurrentResets();
        ASSERT_EQ(Component2::resets_count, expected_resets_count);
        ASSERT_EQ(++Component3::resets_count, expected_resets_count);
    }

    void OnAllComponentsLoaded() override {
        expected_resets_count = 1;
        cc_.ResetCaches(
            cache::UpdateType::kFull,
            {
                std::string{Component1::kName},
                std::string{Component1a::kName},
                std::string{Component1b::kName},
                std::string{Component1c::kName},
                std::string{Component2::kName},
                std::string{Component3::kName},
            },
            {}
        );

        ++expected_resets_count;
        cc_.ResetAllCaches(cache::UpdateType::kFull, {}, {});
    }

    static inline std::atomic<std::size_t> resets_count{0};

private:
    testsuite::CacheControl& cc_;

    // Subscriptions must be the last fields.
    testsuite::CacheResetRegistration reset_registration_;
};

void AssertConcurrentResets() {
    static engine::Mutex block_updates_mutex;
    static engine::ConditionVariable updates_cond_var;

    const auto deadline = engine::Deadline::FromDuration(utest::kMaxTestWaitTime / 2);

    {
        std::unique_lock lock{block_updates_mutex};
        updates_cond_var.NotifyAll();
    }

    std::unique_lock lock{block_updates_mutex};
    updates_cond_var.WaitUntil(lock, deadline, [] {
        bool result =
            Component1::resets_count == expected_resets_count && Component1a::resets_count == expected_resets_count &&
            Component1b::resets_count == expected_resets_count && Component1c::resets_count == expected_resets_count;
        if (!result) {
            UASSERT_MSG(
                Component2::resets_count == expected_resets_count - 1,
                "Dependent Component2 was updated before Component1*"
            );
            UASSERT_MSG(
                Component3::resets_count == expected_resets_count - 1,
                "Dependent Component3 was updated before Component1*"
            );
        }

        return result;
    });

    ASSERT_EQ(Component1::resets_count, expected_resets_count);
    ASSERT_EQ(Component1a::resets_count, expected_resets_count);
    ASSERT_EQ(Component1b::resets_count, expected_resets_count);
    ASSERT_EQ(Component1c::resets_count, expected_resets_count);
}

}  // namespace

template <>
inline constexpr auto components::kConfigFileMode<MyCache> = ConfigFileMode::kNotRequired;

template <>
inline constexpr auto components::kConfigFileMode<Component1> = ConfigFileMode::kNotRequired;

template <>
inline constexpr auto components::kConfigFileMode<Component1a> = ConfigFileMode::kNotRequired;

template <>
inline constexpr auto components::kConfigFileMode<Component1b> = ConfigFileMode::kNotRequired;

template <>
inline constexpr auto components::kConfigFileMode<Component1c> = ConfigFileMode::kNotRequired;

template <>
inline constexpr auto components::kConfigFileMode<Component2> = ConfigFileMode::kNotRequired;

template <>
inline constexpr auto components::kConfigFileMode<Component3> = ConfigFileMode::kNotRequired;

namespace {

constexpr std::string_view kStaticConfigBase = R"(
components_manager:
  event_thread_pool:
    threads: 1
  default_task_processor: main-task-processor
  task_processors:
    main-task-processor:
      worker_threads: 1
  components:
    component-not-loaded:
      load-enabled: false
    logging:
      fs-task-processor: main-task-processor
      loggers:
        default:
          file_path: '@null'

    testsuite-support:
      cache-update-execution: concurrent
)";

components::ComponentList MakeComponentList() {
    return components::ComponentList()
        .Append<os_signals::ProcessorComponent>()
        .Append<components::StatisticsStorage>()
        .Append<components::Logging>()
        .Append<components::Tracer>()
        .Append<Component2>()
        .Append<Component1>()
        .Append<ComponentNotLoaded>()
        .Append<Component3>()
        .Append<components::TestsuiteSupport>()
        .Append<Component1a>()
        .Append<Component1b>()
        .Append<Component1c>()
        .Append<MyCache>()
        .Append<alerts::StorageComponent>();
}

TEST_F(ComponentList, CacheControlConcurrentInvalidation) {
    components::RunOnce(components::InMemoryConfig{std::string{kStaticConfigBase}}, MakeComponentList());
}

}  // namespace

USERVER_NAMESPACE_END
