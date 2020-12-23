#include <utest/utest.hpp>

#include <chrono>

#include <cache/cache_config.hpp>
#include <cache/cache_update_trait.hpp>
#include <cache/test_helpers.hpp>
#include <testsuite/cache_control.hpp>
#include <utils/from_string.hpp>

namespace {

class FakeCache : public cache::CacheUpdateTrait {
 public:
  FakeCache(cache::CacheConfigStatic&& config,
            testsuite::CacheControl& cache_control)
      : cache::CacheUpdateTrait(std::move(config), cache_control, "test",
                                engine::current_task::GetTaskProcessor()) {}

  cache::UpdateType LastUpdateType() const { return last_update_type_; }

  using cache::CacheUpdateTrait::StartPeriodicUpdates;
  using cache::CacheUpdateTrait::StopPeriodicUpdates;

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

const std::string kConfigContents = R"(
update-interval: 10h
update-jitter: 10h
full-update-interval: 10h
additional-cleanup-interval: 10h
)";

}  // namespace

TEST(CacheUpdateTrait, FirstIsFull) {
  RunInCoro([] {
    testsuite::CacheControl cache_control(
        testsuite::CacheControl::PeriodicUpdatesMode::kDisabled);
    FakeCache test_cache(cache::ConfigFromYaml(kConfigContents, "", ""),
                         cache_control);

    test_cache.StartPeriodicUpdates();
    test_cache.StopPeriodicUpdates();
    EXPECT_EQ(cache::UpdateType::kFull, test_cache.LastUpdateType());
  });
}
