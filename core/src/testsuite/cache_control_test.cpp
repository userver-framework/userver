#include <utest/utest.hpp>

#include <chrono>

#include <cache/cache_config.hpp>
#include <cache/cache_update_trait.hpp>
#include <testsuite/cache_control.hpp>

namespace {

static std::string kCacheName = "test_cache";

class FakeCache : public cache::CacheUpdateTrait {
 public:
  FakeCache(cache::CacheConfig&& config, testsuite::CacheControl& cache_control)
      : cache::CacheUpdateTrait(std::move(config), cache_control, kCacheName) {
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

TEST(CacheControl, Smoke) {
  RunInCoro([] {
    testsuite::CacheControl cache_control(
        testsuite::CacheControl::PeriodicUpdatesMode::kDisabled);
    FakeCache test_cache(
        cache::CacheConfig(std::chrono::seconds{1}, std::chrono::seconds{1},
                           std::chrono::seconds{1}, std::chrono::seconds{1}),
        cache_control);

    EXPECT_EQ(0, test_cache.UpdatesCount());

    cache_control.InvalidateCaches(cache::UpdateType::kFull, {kCacheName});
    EXPECT_EQ(1, test_cache.UpdatesCount());
    EXPECT_EQ(cache::UpdateType::kFull, test_cache.LastUpdateType());

    cache_control.InvalidateCaches(cache::UpdateType::kIncremental,
                                   {"not_" + kCacheName});
    EXPECT_EQ(1, test_cache.UpdatesCount());
    EXPECT_EQ(cache::UpdateType::kFull, test_cache.LastUpdateType());

    cache_control.InvalidateCaches(cache::UpdateType::kIncremental, {});
    EXPECT_EQ(1, test_cache.UpdatesCount());
    EXPECT_EQ(cache::UpdateType::kFull, test_cache.LastUpdateType());

    cache_control.InvalidateAllCaches(cache::UpdateType::kIncremental);
    EXPECT_EQ(2, test_cache.UpdatesCount());
    EXPECT_EQ(cache::UpdateType::kIncremental, test_cache.LastUpdateType());
  });
}
