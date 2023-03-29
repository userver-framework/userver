#include <userver/cache/cache_update_trait.hpp>

#include <chrono>
#include <exception>
#include <optional>
#include <utility>
#include <vector>

#include <fmt/format.h>
#include <boost/filesystem.hpp>

#include <cache/internal_helpers_test.hpp>
#include <dump/internal_helpers_test.hpp>
#include <userver/cache/cache_config.hpp>
#include <userver/cache/update_type.hpp>
#include <userver/components/component.hpp>
#include <userver/dump/common.hpp>
#include <userver/dump/test_helpers.hpp>
#include <userver/dynamic_config/storage_mock.hpp>
#include <userver/engine/sleep.hpp>
#include <userver/formats/yaml/serialize.hpp>
#include <userver/formats/yaml/value_builder.hpp>
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

  ~FakeCache() final { StopPeriodicUpdates(); }

  cache::UpdateType LastUpdateType() const { return last_update_type_; }

 private:
  void Update(cache::UpdateType type,
              const std::chrono::system_clock::time_point& last_update,
              const std::chrono::system_clock::time_point& now,
              cache::UpdateStatisticsScope&) override {
    EXPECT_EQ(last_update, std::chrono::system_clock::time_point{})
        << "Guarantee in docs of cache::CacheUpdateTrait::Update is broken";
    EXPECT_NE(now, std::chrono::system_clock::time_point{});
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

  ~DumpedCache() final { StopPeriodicUpdates(); }

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

const auto kAnyDumpAvailable =
    Values(DumpAvailable{false}, DumpAvailable{true});

const auto kAnyDataSourceAvailable =
    Values(DataSourceAvailable{false}, DataSourceAvailable{true});

yaml_config::YamlConfig MakeDumpedCacheConfig(const TestParams& params) {
  const auto& [update_types, first_update_mode, first_update_type, dump_exists,
               data_source_exists] = params;

  static constexpr std::string_view kConfigTemplate = R"(
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

yaml_config::YamlConfig UpdateConfig(yaml_config::YamlConfig& config,
                                     formats::yaml::Value&& other) {
  formats::yaml::ValueBuilder builder(config.Yaml());
  formats::yaml::ValueBuilder yaml{other};
  for (const auto& [name, value] : Items(yaml)) {
    builder[name] = value;
  }

  return {builder.ExtractValue(), formats::yaml::Value{} /*config_vars*/};
}

class CacheUpdateTraitDumped : public ::testing::TestWithParam<TestParams> {
 protected:
  CacheUpdateTraitDumped() { InitDumpAndData(); }

  explicit CacheUpdateTraitDumped(
      testsuite::CacheControl::PeriodicUpdatesMode update_mode)
      : environment_(update_mode) {
    InitDumpAndData();
  }

  static std::string ParamsString() {
    return fmt::format("Params({}, {}, {}, {}, {})",
                       ToString(std::get<AllowedUpdateTypes>(GetParam())),
                       ToString(std::get<FirstUpdateMode>(GetParam())),
                       ToString(std::get<FirstUpdateType>(GetParam())),
                       std::get<DumpAvailable>(GetParam()),
                       std::get<DataSourceAvailable>(GetParam()));
  }

  void InitDumpAndData() {
    if (std::get<DumpAvailable>(GetParam())) {
      dump::CreateDump(dump::ToBinary(std::uint64_t{10}),
                       {DumpedCache::kName, config_[dump::kDump],
                        environment_.dump_root.GetPath()});
    }

    if (std::get<DataSourceAvailable>(GetParam())) {
      data_source_.Set(20);
    }
  }

  yaml_config::YamlConfig& Config() { return config_; }
  cache::MockEnvironment& GetEnvironment() { return environment_; }
  cache::DataSourceMock<std::uint64_t>& GetDataSource() { return data_source_; }

 private:
  yaml_config::YamlConfig config_ = MakeDumpedCacheConfig(GetParam());
  cache::MockEnvironment environment_;
  cache::DataSourceMock<std::uint64_t> data_source_{{}};
};

class CacheUpdateTraitDumpedNoUpdate : public CacheUpdateTraitDumped {};
class CacheUpdateTraitDumpedFull : public CacheUpdateTraitDumped {};
class CacheUpdateTraitDumpedIncremental : public CacheUpdateTraitDumped {};
class CacheUpdateTraitDumpedFailure : public CacheUpdateTraitDumped {};
class CacheUpdateTraitDumpedFailureOk : public CacheUpdateTraitDumped {};
class CacheUpdateTraitDumpedIncrementalThenAsyncFull
    : public CacheUpdateTraitDumped {
 public:
  CacheUpdateTraitDumpedIncrementalThenAsyncFull()
      : CacheUpdateTraitDumped(
            testsuite::CacheControl::PeriodicUpdatesMode::kEnabled) {
    Config() = UpdateConfig(Config(),
                            formats::yaml::FromString("update-interval: 1ms"));
  }
};

}  // namespace

UTEST_P(CacheUpdateTraitDumpedIncrementalThenAsyncFull, Test) {
  DumpedCache cache(Config(), GetEnvironment(), GetDataSource());

  // There will be no data race because only one thread is using
  while (cache.GetUpdatesLog().size() < 3) {
    engine::Yield();
  }

  size_t updates = cache.GetUpdatesLog().size();

  GetDataSource().Set(20);

  while (cache.GetUpdatesLog().size() < updates + 2) {
    engine::Yield();
  }

  const auto& updates_log = cache.GetUpdatesLog();
  EXPECT_GE(updates_log.size(), 3);
  // Data not available. Incremental fail
  EXPECT_EQ(updates_log[0], UpdateType::kIncremental);
  // Data not available. Full fail
  EXPECT_EQ(updates_log[1], UpdateType::kFull);
  // Data not available. Retry Full fail
  EXPECT_EQ(updates_log[2], UpdateType::kFull);
  //...
  // Data available. Retry Full success
  EXPECT_EQ(updates_log[updates], UpdateType::kFull);
  // Data available. Incremental success
  EXPECT_EQ(updates_log[updates + 1], UpdateType::kIncremental);
}

INSTANTIATE_UTEST_SUITE_P(
    FullFailure, CacheUpdateTraitDumpedIncrementalThenAsyncFull,
    Combine(Values(AllowedUpdateTypes::kOnlyIncremental),
            Values(FirstUpdateMode::kBestEffort),
            Values(FirstUpdateType::kIncrementalThenAsyncFull),
            Values(DumpAvailable{true}), Values(DataSourceAvailable{false})));

UTEST_P(CacheUpdateTraitDumpedNoUpdate, Test) {
  try {
    DumpedCache cache{Config(), GetEnvironment(), GetDataSource()};
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
    DumpedCache cache{Config(), GetEnvironment(), GetDataSource()};
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
  UEXPECT_NO_THROW(cache.emplace(Config(), GetEnvironment(), GetDataSource()))
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

UTEST_P(CacheUpdateTraitDumpedFailureOk, Test) {
  try {
    Config() = UpdateConfig(
        Config(), formats::yaml::FromString("first-update-fail-ok: true"));
    DumpedCache cache{Config(), GetEnvironment(), GetDataSource()};
    SUCCEED();
  } catch (const cache::MockError&) {
    FAIL() << ParamsString();
  } catch (const cache::ConfigError&) {
    FAIL() << ParamsString();
  }
}

// 1. Fails or not to load data from dump
// 2. Requests a synchronous full update
// 3. The synchronous full update fails
// 4. Cache starts successfully
INSTANTIATE_UTEST_SUITE_P(
    UnnecessaryCacheStart, CacheUpdateTraitDumpedFailureOk,
    Combine(Values(AllowedUpdateTypes::kFullAndIncremental),
            Values(FirstUpdateMode::kBestEffort, FirstUpdateMode::kSkip),
            kAnyFirstUpdateType, kAnyDumpAvailable,
            Values(DataSourceAvailable{false})));

UTEST_P(CacheUpdateTraitDumpedFailure, Test) {
  try {
    DumpedCache cache{Config(), GetEnvironment(), GetDataSource()};
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

  ~FaultyDumpedCache() final { StopPeriodicUpdates(); }

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

namespace {

class ExpirableCache : public cache::CacheMockBase {
 public:
  static constexpr auto kName = "expirable-cache";

  ExpirableCache(const yaml_config::YamlConfig& config,
                 cache::MockEnvironment& environment,
                 std::function<bool(std::uint64_t)> is_update_failed)
      : CacheMockBase(kName, config, environment),
        is_update_failed_(std::move(is_update_failed)) {
    StartPeriodicUpdates();
  }

  ~ExpirableCache() override { StopPeriodicUpdates(); }

  const auto& GetExpiredLog() const { return expired_log_; }

 private:
  void Update(cache::UpdateType /*type*/,
              const std::chrono::system_clock::time_point& /*last_update*/,
              const std::chrono::system_clock::time_point& /*now*/,
              cache::UpdateStatisticsScope&) override {
    expired_log_.emplace_back(is_expired_);
    if (is_update_failed_(expired_log_.size())) throw cache::MockError();
    is_expired_ = false;
  }

  void MarkAsExpired() override { is_expired_ = true; }

  std::function<bool(std::uint64_t)> is_update_failed_;
  std::vector<bool> expired_log_;
  bool is_expired_ = false;
};

yaml_config::YamlConfig MakeExpirableCacheConfig(std::uint64_t expired_number) {
  static constexpr std::string_view kConfigTemplate = R"(
update-types: only-incremental
update-interval: 1ms
failed-updates-before-expiration: {}
first-update-fail-ok: true
)";
  return {
      formats::yaml::FromString(fmt::format(kConfigTemplate, expired_number)),
      {}};
}

}  // namespace

UTEST(ExpirableCacheUpdateTrait, TwoFailed) {
  auto config = MakeExpirableCacheConfig(2);
  cache::MockEnvironment environment(
      testsuite::CacheControl::PeriodicUpdatesMode::kEnabled);
  ExpirableCache cache(config, environment, [](auto i) -> bool {
    std::vector<int> failed{1, 3, 4, 5, 7, 9, 10, 11};
    return std::count(failed.begin(), failed.end(), i);
  });

  while (cache.GetExpiredLog().size() < 13) {
    engine::Yield();
  }

  const auto& actual = cache.GetExpiredLog();
  const std::vector<bool> expected{false, false, false, false, true,
                                   true,  false, false, false, false,
                                   true,  true,  false};
  EXPECT_EQ(actual, expected);
}

USERVER_NAMESPACE_END
