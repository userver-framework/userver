#include <userver/utest/utest.hpp>

#include <chrono>
#include <optional>
#include <string>

#include <cache/internal_helpers_test.hpp>
#include <dump/internal_helpers_test.hpp>
#include <userver/cache/cache_config.hpp>
#include <userver/cache/cache_update_trait.hpp>
#include <userver/components/loggable_component_base.hpp>
#include <userver/concurrent/variable.hpp>
#include <userver/dump/common.hpp>
#include <userver/dump/unsafe.hpp>
#include <userver/formats/yaml/serialize.hpp>
#include <userver/fs/blocking/temp_directory.hpp>
#include <userver/fs/blocking/write.hpp>
#include <userver/testsuite/cache_control.hpp>
#include <userver/yaml_config/yaml_config.hpp>

USERVER_NAMESPACE_BEGIN

namespace {

const std::string kCacheName = "test_cache";
const std::string kCacheNameAlternative = "test_cache_alternative";
const std::string kDumpToRead = "2015-03-22T090000.000000Z-v0";
constexpr std::size_t kDummyDocumentsCount = 42;

class FakeCache final : public cache::CacheMockBase {
 public:
  FakeCache(std::string_view name, const yaml_config::YamlConfig& config,
            cache::MockEnvironment& environment)
      : CacheMockBase(name, config, environment) {
    StartPeriodicUpdates(cache::CacheUpdateTrait::Flag::kNoFirstUpdate);
  }

  ~FakeCache() final { StopPeriodicUpdates(); }

  const std::string& Get() const { return value_; }
  size_t UpdatesCount() const { return updates_count_; }
  cache::UpdateType LastUpdateType() const { return last_update_type_; }

 private:
  void Update(cache::UpdateType type,
              const std::chrono::system_clock::time_point&,
              const std::chrono::system_clock::time_point&,
              cache::UpdateStatisticsScope& stats_scope) override {
    ++updates_count_;
    last_update_type_ = type;
    value_ = "foo";
    OnCacheModified();
    stats_scope.Finish(kDummyDocumentsCount);
  }

  void GetAndWrite(dump::Writer& writer) const override {
    dump::WriteStringViewUnsafe(writer, value_);
  }

  void ReadAndSet(dump::Reader& reader) override {
    value_ = dump::ReadEntire(reader);
  }

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
  const yaml_config::YamlConfig config{
      formats::yaml::FromString(kConfigContents), {}};
  cache::MockEnvironment env;

  FakeCache test_cache(kCacheName, config, env);

  FakeCache test_cache_alternative(kCacheNameAlternative, config, env);

  // Periodic updates are disabled, so a synchronous update will be performed
  EXPECT_EQ(1, test_cache.UpdatesCount());

  env.cache_control.ResetCaches(cache::UpdateType::kFull, {kCacheName},
                                /*force_incremental_names=*/{});
  EXPECT_EQ(2, test_cache.UpdatesCount());
  EXPECT_EQ(cache::UpdateType::kFull, test_cache.LastUpdateType());

  env.cache_control.ResetCaches(cache::UpdateType::kIncremental,
                                {kCacheNameAlternative},
                                /*force_incremental_names=*/{});
  EXPECT_EQ(2, test_cache.UpdatesCount());
  EXPECT_EQ(cache::UpdateType::kFull, test_cache.LastUpdateType());

  env.cache_control.ResetCaches(cache::UpdateType::kIncremental, {},
                                /*force_incremental_names=*/{});
  EXPECT_EQ(2, test_cache.UpdatesCount());
  EXPECT_EQ(cache::UpdateType::kFull, test_cache.LastUpdateType());

  env.cache_control.ResetAllCaches(cache::UpdateType::kIncremental,
                                   /*force_incremental_names=*/{});
  EXPECT_EQ(3, test_cache.UpdatesCount());
  EXPECT_EQ(cache::UpdateType::kIncremental, test_cache.LastUpdateType());

  EXPECT_EQ(test_cache.Get(), "foo");

  boost::filesystem::remove_all(env.dump_root.GetPath());
  dump::CreateDumps({kDumpToRead}, env.dump_root, kCacheName);
  env.dump_control.ReadCacheDumps({kCacheName});
  EXPECT_EQ(test_cache.Get(), kDumpToRead);

  boost::filesystem::remove_all(env.dump_root.GetPath());
  env.cache_control.ResetCaches(cache::UpdateType::kFull, {kCacheName},
                                /*force_incremental_names=*/{});
  env.dump_control.WriteCacheDumps({kCacheName});
  EXPECT_EQ(dump::FilenamesInDirectory(env.dump_root, kCacheName).size(), 1);
}

UTEST_DEATH(CacheControlDeathTest, MissingCache) {
  const yaml_config::YamlConfig config{
      formats::yaml::FromString(kConfigContents), {}};
  cache::MockEnvironment env;
  FakeCache test_cache(kCacheName, config, env);

  EXPECT_UINVARIANT_FAILURE(env.dump_control.WriteCacheDumps({"missing"}));
  EXPECT_UINVARIANT_FAILURE(env.dump_control.ReadCacheDumps({"missing"}));
}

namespace {

/// [sample]
class MyCache final : public components::LoggableComponentBase {
 public:
  static constexpr std::string_view kName = "my-cache";

  MyCache(const components::ComponentConfig& config,
          const components::ComponentContext& context)
      : components::LoggableComponentBase(config, context) {
    {
      auto cache = cached_token_.Lock();
      *cache = FetchToken();
    }
    // ...

    // reset_registration_ must be set at the end of the constructor.
    reset_registration_ = testsuite::FindCacheControl(context).RegisterCache(
        this, kName, &MyCache::ResetCache);
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

}  // namespace

USERVER_NAMESPACE_END
