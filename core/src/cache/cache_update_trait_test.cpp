#include <utest/utest.hpp>

#include <chrono>

#include <cache/cache_config.hpp>
#include <cache/cache_update_trait.hpp>
#include <testsuite/cache_control.hpp>

namespace {

class FakeCache : public cache::CacheUpdateTrait {
 public:
  FakeCache(cache::CacheConfig&& config, testsuite::CacheControl& cache_control)
      : cache::CacheUpdateTrait(std::move(config), cache_control, "test") {}

  using cache::CacheUpdateTrait::DoPeriodicUpdate;

  cache::UpdateType LastUpdateType() const { return last_update_type_; }

 private:
  void Update(cache::UpdateType type,
              const std::chrono::system_clock::time_point&,
              const std::chrono::system_clock::time_point&,
              cache::UpdateStatisticsScope&) override {
    last_update_type_ = type;
  }

  void Cleanup() override {}

  cache::UpdateType last_update_type_{cache::UpdateType::kIncremental};
};

}  // namespace

TEST(CacheUpdateTrait, FirstIsFull) {
  RunInCoro([] {
    testsuite::CacheControl cache_control(
        testsuite::CacheControl::PeriodicUpdatesMode::kDisabled);
    FakeCache test_cache(
        cache::CacheConfig(std::chrono::seconds{1}, std::chrono::seconds{1},
                           std::chrono::milliseconds::max(),
                           std::chrono::milliseconds(100)),
        cache_control);
    test_cache.DoPeriodicUpdate();
    EXPECT_EQ(cache::UpdateType::kFull, test_cache.LastUpdateType());
  });
}
