#include <userver/utest/utest.hpp>

#include <chrono>

#include <cache/internal_helpers_test.hpp>
#include <dump/internal_helpers_test.hpp>
#include <userver/cache/cache_config.hpp>
#include <userver/cache/cache_update_trait.hpp>
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
              cache::UpdateStatisticsScope&) override {
    ++updates_count_;
    last_update_type_ = type;
    value_ = "foo";
    OnCacheModified();
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

UTEST_DEATH(CacheControlDeathTest, Smoke) {
  const yaml_config::YamlConfig config{
      formats::yaml::FromString(kConfigContents), {}};
  cache::MockEnvironment env;

  FakeCache test_cache(kCacheName, config, env);

  FakeCache test_cache_alternative(kCacheNameAlternative, config, env);

  // Periodic updates are disabled, so a synchronous update will be performed
  EXPECT_EQ(1, test_cache.UpdatesCount());

  env.cache_control.InvalidateCaches(cache::UpdateType::kFull, {kCacheName});
  EXPECT_EQ(2, test_cache.UpdatesCount());
  EXPECT_EQ(cache::UpdateType::kFull, test_cache.LastUpdateType());

  env.cache_control.InvalidateCaches(cache::UpdateType::kIncremental,
                                     {kCacheNameAlternative});
  EXPECT_EQ(2, test_cache.UpdatesCount());
  EXPECT_EQ(cache::UpdateType::kFull, test_cache.LastUpdateType());

  env.cache_control.InvalidateCaches(cache::UpdateType::kIncremental, {});
  EXPECT_EQ(2, test_cache.UpdatesCount());
  EXPECT_EQ(cache::UpdateType::kFull, test_cache.LastUpdateType());

  env.cache_control.InvalidateAllCaches(cache::UpdateType::kIncremental);
  EXPECT_EQ(3, test_cache.UpdatesCount());
  EXPECT_EQ(cache::UpdateType::kIncremental, test_cache.LastUpdateType());

  EXPECT_EQ(test_cache.Get(), "foo");

  boost::filesystem::remove_all(env.dump_root.GetPath());
  dump::CreateDumps({kDumpToRead}, env.dump_root, kCacheName);
  env.dump_control.ReadCacheDumps({kCacheName});
  EXPECT_EQ(test_cache.Get(), kDumpToRead);

  boost::filesystem::remove_all(env.dump_root.GetPath());
  env.cache_control.InvalidateCaches(cache::UpdateType::kFull, {kCacheName});
  env.dump_control.WriteCacheDumps({kCacheName});
  EXPECT_EQ(dump::FilenamesInDirectory(env.dump_root, kCacheName).size(), 1);

  EXPECT_UINVARIANT_FAILURE(env.dump_control.WriteCacheDumps({"missing"}));
  EXPECT_UINVARIANT_FAILURE(env.dump_control.ReadCacheDumps({"missing"}));
}

USERVER_NAMESPACE_END
