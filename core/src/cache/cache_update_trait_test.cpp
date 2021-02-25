#include <utest/utest.hpp>

#include <chrono>
#include <stdexcept>

#include <fmt/format.h>

#include <cache/cache_config.hpp>
#include <cache/cache_update_trait.hpp>
#include <cache/dump/common.hpp>
#include <cache/dump/dump_manager.hpp>
#include <cache/dump/test_helpers.hpp>
#include <cache/test_helpers.hpp>
#include <fs/blocking/temp_directory.hpp>
#include <fs/blocking/write.hpp>
#include <testsuite/cache_control.hpp>

namespace {

class FakeCache : public cache::CacheUpdateTrait {
 public:
  FakeCache(cache::CacheConfigStatic&& config,
            testsuite::CacheControl& cache_control)
      : cache::CacheUpdateTrait(config, cache_control, "test",
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

const std::string kFakeCacheConfig = R"(
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
    FakeCache test_cache(cache::ConfigFromYaml(kFakeCacheConfig, "", ""),
                         cache_control);

    test_cache.StartPeriodicUpdates();
    test_cache.StopPeriodicUpdates();
    EXPECT_EQ(cache::UpdateType::kFull, test_cache.LastUpdateType());
  });
}

namespace {

class FakeError : public std::runtime_error {
 public:
  FakeError() : std::runtime_error("Simulating an update error") {}
};

class DumpedCache : public cache::CacheUpdateTrait {
 public:
  static constexpr const char* kName = "dumped-cache";

  DumpedCache(cache::CacheConfigStatic&& config,
              testsuite::CacheControl& cache_control,
              bool simulate_update_failure)
      : cache::CacheUpdateTrait(std::move(config), cache_control, kName,
                                engine::current_task::GetTaskProcessor()),
        simulate_update_failure_(simulate_update_failure) {
    StartPeriodicUpdates();
  }

  ~DumpedCache() { StopPeriodicUpdates(); }

  std::uint64_t Get() const { return value_; }

 private:
  void Update(cache::UpdateType, const std::chrono::system_clock::time_point&,
              const std::chrono::system_clock::time_point&,
              cache::UpdateStatisticsScope&) override {
    if (simulate_update_failure_) throw FakeError{};
    ++value_;
  }

  void GetAndWrite(cache::dump::Writer& writer) const override {
    writer.Write(value_);
  }

  void ReadAndSet(cache::dump::Reader& reader) override {
    value_ = reader.Read<std::uint64_t>();
  }

  void Cleanup() override {}

  std::uint64_t value_{0};
  bool simulate_update_failure_;
};

const std::string kDumpedCacheConfig = R"(
update-type: only-incremental
update-interval: 1s
dump:
    enable: true
    world-readable: true
    format-version: 0
    first-update-mode: {first_update_mode}
    max-age:  # unlimited
    force-full-second-update: false
)";

struct DumpedCacheTestParams {
  bool valid_dump_present;
  std::string first_update_mode;
  bool simulate_update_failure;
  std::optional<std::uint64_t> expected_initial_value;
};

template <typename T>
std::string ToString(const std::optional<T>& value) {
  return value ? fmt::to_string(*value) : "(null)";
}

[[maybe_unused]] void PrintTo(const DumpedCacheTestParams& params,
                              std::ostream* out) {
  *out << fmt::format("DumpedCacheTestParams({}, {}, {}, {})",
                      params.valid_dump_present, params.first_update_mode,
                      params.simulate_update_failure,
                      ToString(params.expected_initial_value));
}

class CacheUpdateTraitDumped
    : public ::testing::TestWithParam<DumpedCacheTestParams> {
 protected:
  void SetUp() override {
    dump_root_ = fs::blocking::TempDirectory::Create();
    config_ = cache::ConfigFromYaml(
        fmt::format(kDumpedCacheConfig, fmt::arg("first_update_mode",
                                                 GetParam().first_update_mode)),
        dump_root_.GetPath(), DumpedCache::kName);

    if (GetParam().valid_dump_present) {
      cache::dump::DumpManager dump_manager(cache::CacheConfigStatic{config_},
                                            DumpedCache::kName);
      const auto dump_stats =
          dump_manager.RegisterNewDump(cache::dump::TimePoint{});
      fs::blocking::RewriteFileContents(
          dump_stats.full_path, cache::dump::ToBinary(std::uint64_t{42}));
    }
  }

  fs::blocking::TempDirectory dump_root_;
  cache::CacheConfigStatic config_{
      cache::ConfigFromYaml(kFakeCacheConfig, "", "")};
  testsuite::CacheControl control_{
      testsuite::CacheControl::PeriodicUpdatesMode::kDisabled};
};

class CacheUpdateTraitDumpedSuccess : public CacheUpdateTraitDumped {};
class CacheUpdateTraitDumpedFailure : public CacheUpdateTraitDumped {};

}  // namespace

TEST_P(CacheUpdateTraitDumpedSuccess, Test) {
  RunInCoro([this] {
    const auto cache = DumpedCache(cache::CacheConfigStatic{config_}, control_,
                                   GetParam().simulate_update_failure);
    EXPECT_EQ(cache.Get(), GetParam().expected_initial_value);
  });
}

TEST_P(CacheUpdateTraitDumpedFailure, Test) {
  RunInCoro([this] {
    EXPECT_THROW(DumpedCache(cache::CacheConfigStatic{config_}, control_,
                             GetParam().simulate_update_failure),
                 FakeError);
  });
}

INSTANTIATE_TEST_SUITE_P(
    FirstUpdateMode, CacheUpdateTraitDumpedSuccess,
    // valid_dump_present, first_update_mode,
    // simulate_update_failure, expected_initial_value,
    ::testing::Values(DumpedCacheTestParams{false, "skip", false, {1}},
                      DumpedCacheTestParams{true, "skip", false, {42}},
                      DumpedCacheTestParams{true, "skip", true, {42}},
                      DumpedCacheTestParams{true, "best-effort", false, {43}},
                      DumpedCacheTestParams{true, "best-effort", true, {42}},
                      DumpedCacheTestParams{true, "required", false, {43}}));

INSTANTIATE_TEST_SUITE_P(
    FirstUpdateMode, CacheUpdateTraitDumpedFailure,
    // valid_dump_present, first_update_mode,
    // simulate_update_failure, expected_initial_value,
    ::testing::Values(DumpedCacheTestParams{false, "skip", true, {}},
                      DumpedCacheTestParams{true, "required", true, {}}));
