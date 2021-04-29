#include <utest/utest.hpp>

#include <chrono>
#include <optional>
#include <stdexcept>
#include <utility>

#include <fmt/format.h>
#include <boost/filesystem.hpp>

#include <cache/cache_config.hpp>
#include <cache/cache_update_trait.hpp>
#include <cache/internal_test_helpers.hpp>
#include <dump/common.hpp>
#include <dump/config.hpp>
#include <dump/dump_locator.hpp>
#include <dump/test_helpers.hpp>
#include <fs/blocking/temp_directory.hpp>
#include <fs/blocking/write.hpp>
#include <testsuite/cache_control.hpp>
#include <testsuite/dump_control.hpp>

namespace {

class FakeCache final : public cache::CacheMockBase {
 public:
  static constexpr auto kName = "fake-cache";

  FakeCache(const components::ComponentConfig& config,
            testsuite::CacheControl& cache_control,
            testsuite::DumpControl& dump_control)
      : CacheMockBase(config, cache_control, dump_control) {
    StartPeriodicUpdates();
  }

  ~FakeCache() { StopPeriodicUpdates(); }

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

const std::string kFakeCacheConfig = R"(
update-interval: 10h
update-jitter: 10h
full-update-interval: 10h
additional-cleanup-interval: 10h
)";

}  // namespace

TEST(CacheUpdateTrait, FirstIsFull) {
  RunInCoro([] {
    const auto dump_root = fs::blocking::TempDirectory::Create();
    testsuite::CacheControl cache_control(
        testsuite::CacheControl::PeriodicUpdatesMode::kDisabled);
    testsuite::DumpControl dump_control;

    FakeCache test_cache(
        cache::ConfigFromYaml(kFakeCacheConfig, dump_root, FakeCache::kName),
        cache_control, dump_control);

    EXPECT_EQ(cache::UpdateType::kFull, test_cache.LastUpdateType());
  });
}

namespace {

class DumpedCache final : public cache::CacheMockBase {
 public:
  static constexpr const char* kName = "dumped-cache";

  DumpedCache(const components::ComponentConfig& config,
              testsuite::CacheControl& cache_control,
              testsuite::DumpControl& dump_control,
              cache::DataSourceMock<std::uint64_t>& data_source)
      : cache::CacheMockBase(config, cache_control, dump_control),
        data_source_(data_source) {
    StartPeriodicUpdates();
  }

  ~DumpedCache() { StopPeriodicUpdates(); }

  std::uint64_t Get() const { return value_; }

 private:
  void Update(cache::UpdateType, const std::chrono::system_clock::time_point&,
              const std::chrono::system_clock::time_point&,
              cache::UpdateStatisticsScope&) override {
    const auto new_value = data_source_.Fetch();
    if (value_ == new_value) return;
    value_ = new_value;
    OnCacheModified();
  }

  void GetAndWrite(dump::Writer& writer) const override {
    writer.Write(value_);
  }

  void ReadAndSet(dump::Reader& reader) override {
    value_ = reader.Read<std::uint64_t>();
  }

  void Cleanup() override {}

  std::uint64_t value_{0};
  cache::DataSourceMock<std::uint64_t>& data_source_;
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
    max-count: 2
    force-full-second-update: false
)";

struct DumpedCacheTestParams {
  bool valid_dump_present;
  std::string first_update_mode;
  std::optional<std::uint64_t> data_source_value;

  std::optional<std::uint64_t> expected_initial_value;
  int expected_update_calls;
};

template <typename T>
std::string ToString(const std::optional<T>& value) {
  return value ? fmt::to_string(*value) : "(null)";
}

[[maybe_unused]] std::ostream& operator<<(std::ostream& out,
                                          const DumpedCacheTestParams& params) {
  out << fmt::format(
      "DumpedCacheTestParams({}, {}, {}, {}, {})", params.valid_dump_present,
      params.first_update_mode, ToString(params.data_source_value),
      ToString(params.expected_initial_value), params.expected_update_calls);
  return out;
}

constexpr std::uint64_t kDataFromDump = 42;

class CacheUpdateTraitDumped
    : public ::testing::TestWithParam<DumpedCacheTestParams> {
 protected:
  void SetUp() override {
    dump_root_ = fs::blocking::TempDirectory::Create();
    config_ = cache::ConfigFromYaml(
        fmt::format(kDumpedCacheConfig, fmt::arg("first_update_mode",
                                                 GetParam().first_update_mode)),
        dump_root_, DumpedCache::kName);

    if (GetParam().valid_dump_present) {
      const auto dump_stats = dump::DumpLocator{}.RegisterNewDump(
          dump::TimePoint{}, dump::Config{config_});
      fs::blocking::RewriteFileContents(dump_stats.full_path,
                                        dump::ToBinary(kDataFromDump));
    }
  }

  fs::blocking::TempDirectory dump_root_;
  components::ComponentConfig config_{{}};
  testsuite::CacheControl cache_control_{
      testsuite::CacheControl::PeriodicUpdatesMode::kDisabled};
  testsuite::DumpControl dump_control_;
};

class CacheUpdateTraitDumpedSuccess : public CacheUpdateTraitDumped {};
class CacheUpdateTraitDumpedFailure : public CacheUpdateTraitDumped {};

}  // namespace

TEST_P(CacheUpdateTraitDumpedSuccess, Test) {
  RunInCoro([this] {
    cache::DataSourceMock<std::uint64_t> data_source(
        GetParam().data_source_value);
    const auto cache =
        DumpedCache(config_, cache_control_, dump_control_, data_source);
    EXPECT_EQ(cache.Get(), GetParam().expected_initial_value) << GetParam();
    EXPECT_EQ(data_source.GetFetchCallsCount(),
              GetParam().expected_update_calls)
        << GetParam();
  });
}

TEST_P(CacheUpdateTraitDumpedFailure, Test) {
  RunInCoro([this] {
    cache::DataSourceMock<std::uint64_t> data_source(
        GetParam().data_source_value);
    EXPECT_THROW(
        DumpedCache(config_, cache_control_, dump_control_, data_source),
        cache::MockError)
        << GetParam();
    EXPECT_EQ(data_source.GetFetchCallsCount(),
              GetParam().expected_update_calls)
        << GetParam();
  });
}

// clang-format off
INSTANTIATE_TEST_SUITE_P(
    FirstUpdateMode, CacheUpdateTraitDumpedSuccess,
    // - valid_dump_present
    // - first_update_mode
    // - data_source_value
    // - expected_initial_value
    // - expected_update_calls
    ::testing::Values(DumpedCacheTestParams{false, "skip",        {5}, {5},  1},
                      DumpedCacheTestParams{true,  "skip",        {5}, {42}, 0},
                      DumpedCacheTestParams{true,  "skip",        {},  {42}, 0},
                      DumpedCacheTestParams{true,  "best-effort", {5}, {5},  1},
                      DumpedCacheTestParams{true,  "best-effort", {},  {42}, 1},
                      DumpedCacheTestParams{true,  "required",    {5}, {5},  1}));

INSTANTIATE_TEST_SUITE_P(
    FirstUpdateMode, CacheUpdateTraitDumpedFailure,
    ::testing::Values(DumpedCacheTestParams{false, "skip",        {},  {},   1},
                      DumpedCacheTestParams{true,  "required",    {},  {},   1}));
// clang-format on

TEST(CacheUpdateTrait, WriteDumps) {
  RunInCoro([] {
    const auto dump_root = fs::blocking::TempDirectory::Create();
    testsuite::CacheControl cache_control{
        testsuite::CacheControl::PeriodicUpdatesMode::kDisabled};
    testsuite::DumpControl dump_control;
    auto config = cache::ConfigFromYaml(
        fmt::format(kDumpedCacheConfig, fmt::arg("first_update_mode", "skip")),
        dump_root, DumpedCache::kName);
    cache::DataSourceMock<std::uint64_t> data_source(5);

    DumpedCache cache(std::move(config), cache_control, dump_control,
                      data_source);

    const auto write_and_count_dumps = [&] {
      dump_control.WriteCacheDumps({cache.Name()});
      return dump::FilenamesInDirectory(dump_root, cache.Name()).size();
    };

    EXPECT_EQ(cache.Get(), 5);
    dump_control.WriteCacheDumps({cache.Name()});
    EXPECT_EQ(write_and_count_dumps(), 1);

    data_source.Set(10);
    EXPECT_EQ(cache.Get(), 5);
    cache_control.InvalidateCaches(cache::UpdateType::kFull, {cache.Name()});
    EXPECT_EQ(cache.Get(), 10);
    EXPECT_EQ(write_and_count_dumps(), 2);

    data_source.Set(15);
    cache_control.InvalidateCaches(cache::UpdateType::kFull, {cache.Name()});
    EXPECT_EQ(write_and_count_dumps(), 3);

    data_source.Set(20);
    cache_control.InvalidateCaches(cache::UpdateType::kFull, {cache.Name()});
    EXPECT_EQ(write_and_count_dumps(), 3);

    boost::filesystem::remove_all(dump_root.GetPath());
    EXPECT_EQ(write_and_count_dumps(), 1);
  });
}

namespace {

class FaultyDumpedCache final : public cache::CacheMockBase {
 public:
  static constexpr auto kName = "faulty-dumped-cache";

  FaultyDumpedCache(const components::ComponentConfig& config,
                    testsuite::CacheControl& cache_control,
                    testsuite::DumpControl& dump_control)
      : cache::CacheMockBase(config, cache_control, dump_control) {
    StartPeriodicUpdates();
  }

  ~FaultyDumpedCache() { StopPeriodicUpdates(); }

 private:
  void Update(cache::UpdateType, const std::chrono::system_clock::time_point&,
              const std::chrono::system_clock::time_point&,
              cache::UpdateStatisticsScope&) override {
    OnCacheModified();
  }

  void GetAndWrite(dump::Writer&) const override { throw cache::MockError(); }

  void ReadAndSet(dump::Reader&) override { throw cache::MockError(); }

  void Cleanup() override {}
};

}  // namespace

class CacheUpdateTraitFaulty : public ::testing::Test {
 protected:
  void SetUp() override {
    dump_root_ = fs::blocking::TempDirectory::Create();
    config_ = cache::ConfigFromYaml(
        fmt::format(kDumpedCacheConfig, fmt::arg("first_update_mode", "skip")),
        dump_root_, FaultyDumpedCache::kName);
  }

  fs::blocking::TempDirectory dump_root_;
  components::ComponentConfig config_{{}};
  testsuite::CacheControl cache_control_{
      testsuite::CacheControl::PeriodicUpdatesMode::kDisabled};
  testsuite::DumpControl dump_control_;
};

TEST_F(CacheUpdateTraitFaulty, DumpDebugHandlesThrow) {
  RunInCoro([this] {
    FaultyDumpedCache cache(config_, cache_control_, dump_control_);

    EXPECT_THROW(dump_control_.WriteCacheDumps({cache.Name()}),
                 cache::MockError);
    EXPECT_THROW(dump_control_.ReadCacheDumps({cache.Name()}), dump::Error);

    EXPECT_NO_THROW(cache_control_.InvalidateCaches(cache::UpdateType::kFull,
                                                    {cache.Name()}));
  });
}

TEST_F(CacheUpdateTraitFaulty, TmpDoNotAccumulate) {
  RunInCoro([this] {
    FaultyDumpedCache cache(config_, cache_control_, dump_control_);

    const auto dump_count = [&] {
      return dump::FilenamesInDirectory(dump_root_, cache.Name()).size();
    };

    // This write will fail, leaving behind a garbage .tmp
    EXPECT_THROW(dump_control_.WriteCacheDumps({cache.Name()}),
                 cache::MockError);
    EXPECT_EQ(dump_count(), 1);

    // Will clean up the previous .tmp, then fail
    EXPECT_THROW(dump_control_.WriteCacheDumps({cache.Name()}),
                 cache::MockError);
    EXPECT_EQ(dump_count(), 1);
  });
}
