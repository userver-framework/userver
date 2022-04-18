#include <userver/cache/cache_update_trait.hpp>

#include <chrono>
#include <optional>
#include <utility>

#include <fmt/format.h>
#include <boost/filesystem.hpp>

#include <cache/internal_helpers_test.hpp>
#include <dump/internal_helpers_test.hpp>
#include <userver/cache/cache_config.hpp>
#include <userver/components/component.hpp>
#include <userver/dump/common.hpp>
#include <userver/dump/test_helpers.hpp>
#include <userver/formats/yaml/serialize.hpp>
#include <userver/fs/blocking/temp_directory.hpp>
#include <userver/fs/blocking/write.hpp>
#include <userver/testsuite/cache_control.hpp>
#include <userver/testsuite/dump_control.hpp>
#include <userver/utest/utest.hpp>
#include <userver/yaml_config/yaml_config.hpp>

USERVER_NAMESPACE_BEGIN

namespace {

class FakeCache final : public cache::CacheMockBase {
 public:
  static constexpr auto kName = "fake-cache";

  FakeCache(const yaml_config::YamlConfig& config,
            cache::MockEnvironment& environment)
      : CacheMockBase(kName, config, environment) {
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

  cache::UpdateType last_update_type_{cache::UpdateType::kIncremental};
};

const std::string kFakeCacheConfig = R"(
update-interval: 10h
update-jitter: 10h
full-update-interval: 10h
additional-cleanup-interval: 10h
)";

}  // namespace

UTEST(CacheUpdateTrait, FirstIsFull) {
  const yaml_config::YamlConfig config{
      formats::yaml::FromString(kFakeCacheConfig), {}};
  cache::MockEnvironment environment;

  FakeCache test_cache(config, environment);

  EXPECT_EQ(cache::UpdateType::kFull, test_cache.LastUpdateType());
}

using cache::AllowedUpdateTypes;
using cache::FirstUpdateMode;
using cache::FirstUpdateType;
using cache::UpdateType;
using ::testing::Combine;
using ::testing::Values;

namespace {

class DumpedCache final : public cache::CacheMockBase {
 public:
  static constexpr auto kName = "dumped-cache";

  DumpedCache(const yaml_config::YamlConfig& config,
              cache::MockEnvironment& environment,
              cache::DataSourceMock<std::uint64_t>& data_source)
      : cache::CacheMockBase(kName, config, environment),
        data_source_(data_source) {
    StartPeriodicUpdates();
  }

  ~DumpedCache() { StopPeriodicUpdates(); }

  std::uint64_t Get() const { return value_; }

  const std::vector<UpdateType>& GetUpdatesLog() const { return updates_log_; }

 private:
  void Update(cache::UpdateType type,
              const std::chrono::system_clock::time_point&,
              const std::chrono::system_clock::time_point&,
              cache::UpdateStatisticsScope&) override {
    updates_log_.push_back(type);
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

  std::uint64_t value_{0};
  std::vector<UpdateType> updates_log_;
  cache::DataSourceMock<std::uint64_t>& data_source_;
};

std::string ToString(UpdateType update_type) {
  switch (update_type) {
    case UpdateType::kFull:
      return "full";
    case UpdateType::kIncremental:
      return "incremental";
  }
  UINVARIANT(false, "Unexpected update type");
}

std::string ToString(AllowedUpdateTypes allowed_update_types) {
  switch (allowed_update_types) {
    case AllowedUpdateTypes::kFullAndIncremental:
      return "full-and-incremental";
    case AllowedUpdateTypes::kOnlyFull:
      return "only-full";
    case AllowedUpdateTypes::kOnlyIncremental:
      return "only-incremental";
  }
  UINVARIANT(false, "Unexpected allowed update type");
}

std::string ToString(FirstUpdateMode first_update_mode) {
  switch (first_update_mode) {
    case FirstUpdateMode::kRequired:
      return "required";
    case FirstUpdateMode::kBestEffort:
      return "best-effort";
    case FirstUpdateMode::kSkip:
      return "skip";
  }
  UINVARIANT(false, "Unexpected first update mode");
}

std::string ToString(FirstUpdateType first_update_type) {
  switch (first_update_type) {
    case FirstUpdateType::kFull:
      return "full";
    case FirstUpdateType::kIncremental:
      return "incremental";
    case FirstUpdateType::kIncrementalThenAsyncFull:
      return "incremental-then-async-full";
  }
  UINVARIANT(false, "Unexpected first update type");
}

}  // namespace
namespace cache {

[[maybe_unused]] void PrintTo(UpdateType value, std::ostream* os) {
  *os << ToString(value);
}

}  // namespace cache
namespace {

using DumpAvailable = utils::StrongTypedef<struct DumpAvailableTag, bool>;

using DataSourceAvailable =
    utils::StrongTypedef<struct DataSourceAvailableTag, bool>;

using TestParams =
    std::tuple<AllowedUpdateTypes, FirstUpdateMode, FirstUpdateType,
               DumpAvailable, DataSourceAvailable>;

const auto kAnyAllowedUpdateType =
    Values(AllowedUpdateTypes::kFullAndIncremental,
           AllowedUpdateTypes::kOnlyFull, AllowedUpdateTypes::kOnlyIncremental);

const auto kAnyFirstUpdateMode =
    Values(FirstUpdateMode::kRequired, FirstUpdateMode::kBestEffort,
           FirstUpdateMode::kSkip);

const auto kAnyFirstUpdateType =
    Values(FirstUpdateType::kFull, FirstUpdateType::kIncremental,
           FirstUpdateType::kIncrementalThenAsyncFull);

const auto kAnyDataSourceAvailable =
    Values(DataSourceAvailable{false}, DataSourceAvailable{true});

yaml_config::YamlConfig MakeDumpedCacheConfig(const TestParams& params) {
  const auto& [update_types, first_update_mode, first_update_type, dump_exists,
               data_source_exists] = params;

  static std::string kConfigTemplate = R"(
update-types: {update_types}
update-interval: 1s
{full_update_interval}
dump:
    enable: true
    world-readable: true
    format-version: 0
    first-update-mode: {first_update_mode}
    first-update-type: {first_update_type}
    max-age:  # unlimited
    max-count: 2
)";

  return {formats::yaml::FromString(fmt::format(
              kConfigTemplate, fmt::arg("update_types", ToString(update_types)),
              fmt::arg("full_update_interval",
                       update_types == AllowedUpdateTypes::kFullAndIncremental
                           ? "full-update-interval: 2s"
                           : ""),
              fmt::arg("first_update_mode", ToString(first_update_mode)),
              fmt::arg("first_update_type", ToString(first_update_type)))),
          {}};
}

class CacheUpdateTraitDumped : public ::testing::TestWithParam<TestParams> {
 protected:
  CacheUpdateTraitDumped() {
    if (std::get<DumpAvailable>(GetParam())) {
      dump::CreateDump(dump::ToBinary(std::uint64_t{10}),
                       {DumpedCache::kName, config_[dump::kDump],
                        environment_.dump_root.GetPath()});
    }

    if (std::get<DataSourceAvailable>(GetParam())) {
      data_source_.Set(20);
    }
  }

  std::string ParamsString() const {
    return fmt::format("Params({}, {}, {}, {}, {})",
                       ToString(std::get<AllowedUpdateTypes>(GetParam())),
                       ToString(std::get<FirstUpdateMode>(GetParam())),
                       ToString(std::get<FirstUpdateType>(GetParam())),
                       std::get<DumpAvailable>(GetParam()),
                       std::get<DataSourceAvailable>(GetParam()));
  }

  yaml_config::YamlConfig config_ = MakeDumpedCacheConfig(GetParam());
  cache::MockEnvironment environment_;
  cache::DataSourceMock<std::uint64_t> data_source_{{}};
};

class CacheUpdateTraitDumpedNoUpdate : public CacheUpdateTraitDumped {};
class CacheUpdateTraitDumpedFull : public CacheUpdateTraitDumped {};
class CacheUpdateTraitDumpedIncremental : public CacheUpdateTraitDumped {};
class CacheUpdateTraitDumpedFailure : public CacheUpdateTraitDumped {};

}  // namespace

UTEST_P(CacheUpdateTraitDumpedNoUpdate, Test) {
  try {
    DumpedCache cache{config_, environment_, data_source_};
    EXPECT_EQ(cache.GetUpdatesLog(), std::vector<UpdateType>{})
        << ParamsString();
  } catch (const cache::ConfigError&) {
    // Some of the tested config combinations are invalid, it's not very
    // important to check for those separately.
    SUCCEED();
  }
}

// 1. Loads data from dump
// 2. Performs no further updates, because first-update-mode: skip
// 3. The cache starts with the data from dump
INSTANTIATE_UTEST_SUITE_P(Skip, CacheUpdateTraitDumpedNoUpdate,
                          Combine(kAnyAllowedUpdateType,
                                  Values(FirstUpdateMode::kSkip),
                                  kAnyFirstUpdateType,
                                  Values(DumpAvailable{true}),
                                  kAnyDataSourceAvailable));

UTEST_P(CacheUpdateTraitDumpedFull, Test) {
  try {
    DumpedCache cache{config_, environment_, data_source_};
    EXPECT_EQ(cache.GetUpdatesLog(), std::vector{UpdateType::kFull})
        << ParamsString();
  } catch (const cache::ConfigError&) {
    // Some of the tested config combinations are invalid, it's not very
    // important to check for those separately.
    SUCCEED();
  }
}

// 1. Fails to load data from dump
// 2. Requests a synchronous full update, regardless of
//    update-types and first-update-type
// 3. The synchronous full update succeeds
// 4. The cache starts successfully
INSTANTIATE_UTEST_SUITE_P(NoDump, CacheUpdateTraitDumpedFull,
                          Combine(kAnyAllowedUpdateType,         //
                                  kAnyFirstUpdateMode,           //
                                  kAnyFirstUpdateType,           //
                                  Values(DumpAvailable{false}),  //
                                  Values(DataSourceAvailable{true})));

// 1. Loads data from dump
// 2. Requests a synchronous full update anyway, because first-update-type: full
// 3. The synchronous full update succeeds
// 4. The cache starts successfully
INSTANTIATE_UTEST_SUITE_P(FullUpdateSuccess, CacheUpdateTraitDumpedFull,
                          Combine(kAnyAllowedUpdateType,
                                  Values(FirstUpdateMode::kRequired,
                                         FirstUpdateMode::kBestEffort),
                                  Values(FirstUpdateType::kFull),  //
                                  Values(DumpAvailable{true}),     //
                                  Values(DataSourceAvailable{true})));

// 1. Loads data from dump
// 2. Requests a synchronous full update anyway, because first-update-type: full
// 3. The synchronous full update fails
// 4. The cache starts with the data from dump,
//    because first-update-mode: best-effort
INSTANTIATE_UTEST_SUITE_P(BestEffortFullUpdateFailure,
                          CacheUpdateTraitDumpedFull,
                          Combine(kAnyAllowedUpdateType,                 //
                                  Values(FirstUpdateMode::kBestEffort),  //
                                  Values(FirstUpdateType::kFull),        //
                                  Values(DumpAvailable{true}),           //
                                  Values(DataSourceAvailable{false})));

UTEST_P(CacheUpdateTraitDumpedIncremental, Test) {
  std::optional<DumpedCache> cache;
  UEXPECT_NO_THROW(cache.emplace(config_, environment_, data_source_))
      << ParamsString();
  EXPECT_EQ(cache->GetUpdatesLog(), std::vector{UpdateType::kIncremental})
      << ParamsString();
}

// 1. Loads data from dump
// 2. Requests a synchronous incremental update,
//    because first-update-type != full
// 3. The synchronous incremental update succeeds
// 4. The cache starts with the data from dump + update
INSTANTIATE_UTEST_SUITE_P(
    FreshData, CacheUpdateTraitDumpedIncremental,
    Combine(Values(AllowedUpdateTypes::kFullAndIncremental,
                   AllowedUpdateTypes::kOnlyIncremental),
            Values(FirstUpdateMode::kRequired, FirstUpdateMode::kBestEffort),
            Values(FirstUpdateType::kIncremental,
                   FirstUpdateType::kIncrementalThenAsyncFull),
            Values(DumpAvailable{true}),  //
            Values(DataSourceAvailable{true})));

// 1. Loads data from dump
// 2. Requests a synchronous incremental update,
//    because first-update-type != full
// 3. The synchronous incremental update fails
// 4. The cache starts with the data from dump,
//    because first-update-mode: best-effort
INSTANTIATE_UTEST_SUITE_P(
    FreshDataNotRequired, CacheUpdateTraitDumpedIncremental,
    Combine(Values(AllowedUpdateTypes::kFullAndIncremental,
                   AllowedUpdateTypes::kOnlyIncremental),
            Values(FirstUpdateMode::kBestEffort),
            Values(FirstUpdateType::kIncremental,
                   FirstUpdateType::kIncrementalThenAsyncFull),
            Values(DumpAvailable{true}),  //
            Values(DataSourceAvailable{false})));

UTEST_P(CacheUpdateTraitDumpedFailure, Test) {
  try {
    DumpedCache{config_, environment_, data_source_};
    FAIL() << ParamsString();
  } catch (const cache::MockError&) {
    SUCCEED();
  } catch (const cache::ConfigError&) {
    // Some of the tested config combinations are invalid, it's not very
    // important to check for those separately.
    SUCCEED();
  }
}

// 1. Fails to load data from dump
// 2. Requests a synchronous full update
// 3. The synchronous full update fails
// 4. The cache constructor throws (the service would terminate)
INSTANTIATE_UTEST_SUITE_P(NoData, CacheUpdateTraitDumpedFailure,
                          Combine(kAnyAllowedUpdateType,  //
                                  kAnyFirstUpdateMode,    //
                                  kAnyFirstUpdateType,
                                  Values(DumpAvailable{false}),
                                  Values(DataSourceAvailable{false})));

// 1. Loads data from dump
// 2. Requests a synchronous full update
// 3. The synchronous full update fails
// 4. The cache constructor throws, because first-update-mode: required
//    (the service would terminate)
INSTANTIATE_UTEST_SUITE_P(FreshDataRequired, CacheUpdateTraitDumpedFailure,
                          Combine(kAnyAllowedUpdateType,
                                  Values(FirstUpdateMode::kRequired),
                                  kAnyFirstUpdateType,
                                  Values(DumpAvailable{true}),
                                  Values(DataSourceAvailable{false})));

namespace {

yaml_config::YamlConfig MakeDefaultDumpedCacheConfig() {
  return MakeDumpedCacheConfig({AllowedUpdateTypes::kOnlyIncremental,
                                FirstUpdateMode::kSkip,
                                FirstUpdateType::kFull,
                                {},
                                {}});
}

}  // namespace

UTEST(CacheUpdateTrait, WriteDumps) {
  const auto config = MakeDefaultDumpedCacheConfig();
  cache::MockEnvironment env;
  cache::DataSourceMock<std::uint64_t> data_source(5);

  DumpedCache cache(config, env, data_source);

  const auto write_and_count_dumps = [&] {
    env.dump_control.WriteCacheDumps({cache.Name()});
    return dump::FilenamesInDirectory(env.dump_root, cache.Name()).size();
  };

  EXPECT_EQ(cache.Get(), 5);
  env.dump_control.WriteCacheDumps({cache.Name()});
  EXPECT_EQ(write_and_count_dumps(), 1);

  data_source.Set(10);
  EXPECT_EQ(cache.Get(), 5);
  env.cache_control.InvalidateCaches(cache::UpdateType::kFull, {cache.Name()});
  EXPECT_EQ(cache.Get(), 10);
  EXPECT_EQ(write_and_count_dumps(), 2);

  data_source.Set(15);
  env.cache_control.InvalidateCaches(cache::UpdateType::kFull, {cache.Name()});
  EXPECT_EQ(write_and_count_dumps(), 3);

  data_source.Set(20);
  env.cache_control.InvalidateCaches(cache::UpdateType::kFull, {cache.Name()});
  EXPECT_EQ(write_and_count_dumps(), 3);

  boost::filesystem::remove_all(env.dump_root.GetPath());
  EXPECT_EQ(write_and_count_dumps(), 1);
}

namespace {

class FaultyDumpedCache final : public cache::CacheMockBase {
 public:
  static constexpr auto kName = "faulty-dumped-cache";

  FaultyDumpedCache(const yaml_config::YamlConfig& config,
                    cache::MockEnvironment& environment)
      : cache::CacheMockBase(kName, config, environment) {
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
};

}  // namespace

class CacheUpdateTraitFaulty : public ::testing::Test {
 protected:
  components::ComponentConfig config_ = MakeDefaultDumpedCacheConfig();
  cache::MockEnvironment env_;
};

UTEST_F(CacheUpdateTraitFaulty, DumpDebugHandlesThrow) {
  FaultyDumpedCache cache(config_, env_);

  UEXPECT_THROW(env_.dump_control.WriteCacheDumps({cache.Name()}),
                cache::MockError);
  UEXPECT_THROW(env_.dump_control.ReadCacheDumps({cache.Name()}), dump::Error);

  UEXPECT_NO_THROW(env_.cache_control.InvalidateCaches(cache::UpdateType::kFull,
                                                       {cache.Name()}));
}

UTEST_F(CacheUpdateTraitFaulty, TmpDoNotAccumulate) {
  FaultyDumpedCache cache(config_, env_);

  const auto dump_count = [&] {
    return dump::FilenamesInDirectory(env_.dump_root, cache.Name()).size();
  };

  // This write will fail, leaving behind a garbage .tmp
  UEXPECT_THROW(env_.dump_control.WriteCacheDumps({cache.Name()}),
                cache::MockError);
  EXPECT_EQ(dump_count(), 1);

  // Will clean up the previous .tmp, then fail
  UEXPECT_THROW(env_.dump_control.WriteCacheDumps({cache.Name()}),
                cache::MockError);
  EXPECT_EQ(dump_count(), 1);
}

USERVER_NAMESPACE_END
