#include <utest/utest.hpp>

#include <chrono>

#include <cache/cache_config.hpp>
#include <cache/cache_update_trait.hpp>
#include <cache/test_helpers.hpp>
#include <dump/common.hpp>
#include <dump/unsafe.hpp>
#include <fs/blocking/temp_directory.hpp>
#include <fs/blocking/write.hpp>
#include <testsuite/cache_control.hpp>

namespace {

const std::string kCacheName = "test_cache";
const std::string kCacheNameAlternative = "test_cache_alternative";
const std::string kDumpToRead = "2015-03-22T09:00:00.000000-v0";

class FakeCache final : public cache::CacheMockBase {
 public:
  FakeCache(const components::ComponentConfig& config,
            testsuite::CacheControl& cache_control,
            testsuite::DumpControl& dump_control)
      : CacheMockBase(config, cache_control, dump_control) {
    StartPeriodicUpdates(cache::CacheUpdateTrait::Flag::kNoFirstUpdate);
  }

  ~FakeCache() { StopPeriodicUpdates(); }

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

  void Cleanup() override {}

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
)";

}  // namespace

TEST(CacheControl, Smoke) {
  testing::FLAGS_gtest_death_test_style = "threadsafe";
  RunInCoro([] {
    const auto dump_dir = fs::blocking::TempDirectory::Create();

    testsuite::CacheControl cache_control(
        testsuite::CacheControl::PeriodicUpdatesMode::kDisabled);
    testsuite::DumpControl dump_control;

    FakeCache test_cache(
        cache::ConfigFromYaml(kConfigContents, dump_dir, kCacheName),
        cache_control, dump_control);

    FakeCache test_cache_alternative(
        cache::ConfigFromYaml(kConfigContents, dump_dir, kCacheNameAlternative),
        cache_control, dump_control);

    // Periodic updates are disabled, so a synchronous update will be performed
    EXPECT_EQ(1, test_cache.UpdatesCount());

    cache_control.InvalidateCaches(cache::UpdateType::kFull, {kCacheName});
    EXPECT_EQ(2, test_cache.UpdatesCount());
    EXPECT_EQ(cache::UpdateType::kFull, test_cache.LastUpdateType());

    cache_control.InvalidateCaches(cache::UpdateType::kIncremental,
                                   {kCacheNameAlternative});
    EXPECT_EQ(2, test_cache.UpdatesCount());
    EXPECT_EQ(cache::UpdateType::kFull, test_cache.LastUpdateType());

    cache_control.InvalidateCaches(cache::UpdateType::kIncremental, {});
    EXPECT_EQ(2, test_cache.UpdatesCount());
    EXPECT_EQ(cache::UpdateType::kFull, test_cache.LastUpdateType());

    cache_control.InvalidateAllCaches(cache::UpdateType::kIncremental, {});
    EXPECT_EQ(3, test_cache.UpdatesCount());
    EXPECT_EQ(cache::UpdateType::kIncremental, test_cache.LastUpdateType());

    EXPECT_EQ(test_cache.Get(), "foo");

    boost::filesystem::remove_all(dump_dir.GetPath());
    cache::CreateDumps({kDumpToRead}, dump_dir, kCacheName);
    dump_control.ReadCacheDumps({kCacheName});
    EXPECT_EQ(test_cache.Get(), kDumpToRead);

    boost::filesystem::remove_all(dump_dir.GetPath());
    cache_control.InvalidateCaches(cache::UpdateType::kFull, {kCacheName});
    dump_control.WriteCacheDumps({kCacheName});
    EXPECT_EQ(cache::FilenamesInDirectory(dump_dir, kCacheName).size(), 1);

    EXPECT_YTX_INVARIANT_FAILURE(dump_control.WriteCacheDumps({"missing"}));
    EXPECT_YTX_INVARIANT_FAILURE(dump_control.ReadCacheDumps({"missing"}));
  });
}
