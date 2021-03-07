#include <utest/utest.hpp>

#include <chrono>

#include <cache/cache_config.hpp>
#include <cache/cache_update_trait.hpp>
#include <cache/test_helpers.hpp>
#include <testsuite/cache_control.hpp>

namespace {

static std::string kCacheName = "test_cache";

class FakeCache : public cache::CacheUpdateTrait {
 public:
  FakeCache(cache::CacheConfigStatic&& config,
            testsuite::CacheControl& cache_control)
      : cache::CacheUpdateTrait(config, cache_control, kCacheName,
                                engine::current_task::GetTaskProcessor()) {
    StartPeriodicUpdates(cache::CacheUpdateTrait::Flag::kNoFirstUpdate);
  }

  ~FakeCache() { StopPeriodicUpdates(); }

  size_t UpdatesCount() const { return updates_count_; }
  cache::UpdateType LastUpdateType() const { return last_update_type_; }

 private:
  void Update(cache::UpdateType type,
              const std::chrono::system_clock::time_point&,
              const std::chrono::system_clock::time_point&,
              cache::UpdateStatisticsScope&) override {
    ++updates_count_;
    last_update_type_ = type;
  }

  void Cleanup() override {}

  size_t updates_count_{0};
  cache::UpdateType last_update_type_{cache::UpdateType::kIncremental};
};

}  // namespace

const std::string kConfigContents = R"(
update-interval: 1s
update-jitter: 1s
full-update-interval: 1s
additional-cleanup-interval: 1s
)";

TEST(CacheControl, Smoke) {
  RunInCoro([] {
    testsuite::CacheControl cache_control(
        testsuite::CacheControl::PeriodicUpdatesMode::kDisabled);
    FakeCache test_cache(cache::ConfigFromYaml(kConfigContents, "", ""),
                         cache_control);

    // Periodic updates are disabled, so a synchronous update will be performed
    EXPECT_EQ(1, test_cache.UpdatesCount());

    cache_control.InvalidateCaches(cache::UpdateType::kFull, {kCacheName});
    EXPECT_EQ(2, test_cache.UpdatesCount());
    EXPECT_EQ(cache::UpdateType::kFull, test_cache.LastUpdateType());

    cache_control.InvalidateCaches(cache::UpdateType::kIncremental,
                                   {"not_" + kCacheName});
    EXPECT_EQ(2, test_cache.UpdatesCount());
    EXPECT_EQ(cache::UpdateType::kFull, test_cache.LastUpdateType());

    cache_control.InvalidateCaches(cache::UpdateType::kIncremental, {});
    EXPECT_EQ(2, test_cache.UpdatesCount());
    EXPECT_EQ(cache::UpdateType::kFull, test_cache.LastUpdateType());

    cache_control.InvalidateAllCaches(cache::UpdateType::kIncremental, {});
    EXPECT_EQ(3, test_cache.UpdatesCount());
    EXPECT_EQ(cache::UpdateType::kIncremental, test_cache.LastUpdateType());
  });
}
