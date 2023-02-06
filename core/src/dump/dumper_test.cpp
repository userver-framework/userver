#include <userver/dump/dumper.hpp>

#include <atomic>
#include <chrono>
#include <unordered_map>
#include <vector>

#include <dump/internal_helpers_test.hpp>
#include <userver/components/loggable_component_base.hpp>
#include <userver/dump/common.hpp>
#include <userver/dump/common_containers.hpp>
#include <userver/dump/factory.hpp>
#include <userver/dump/test_helpers.hpp>
#include <userver/dynamic_config/storage_mock.hpp>
#include <userver/engine/get_all.hpp>
#include <userver/engine/mutex.hpp>
#include <userver/engine/sleep.hpp>
#include <userver/engine/task/task_with_result.hpp>
#include <userver/rcu/rcu_map.hpp>
#include <userver/testsuite/dump_control.hpp>
#include <userver/utest/assert_macros.hpp>
#include <userver/utest/utest.hpp>
#include <userver/utils/async.hpp>
#include <userver/utils/atomic.hpp>
#include <userver/utils/mock_now.hpp>
#include <userver/utils/statistics/storage.hpp>

using namespace std::chrono_literals;

USERVER_NAMESPACE_BEGIN

namespace {

// Don't take this as an example! A good example is SampleComponentWithDumps.
struct DummyEntity final : public dump::DumpableEntity {
  static constexpr auto kName = "dummy";

  void GetAndWrite(dump::Writer& writer) const override {
    std::unique_lock lock(check_no_data_race_mutex, std::try_to_lock);
    ASSERT_TRUE(lock.owns_lock());

    std::lock_guard write_lock(write_mutex);

    writer.Write(value);
    ++write_count;
  }

  void ReadAndSet(dump::Reader& reader) override {
    std::unique_lock lock(check_no_data_race_mutex, std::try_to_lock);
    ASSERT_TRUE(lock.owns_lock());

    value = reader.Read<int>();
    ++read_count;
  }

  int value{0};
  mutable int write_count{0};
  mutable int read_count{0};

  mutable engine::Mutex check_no_data_race_mutex;
  mutable engine::Mutex write_mutex;
};

const std::string kConfig = R"(
enable: true
world-readable: true
format-version: 0
max-age:  # unlimited
max-count: 3
)";

struct DumperFixtureConfig final {
  testsuite::DumpControl::PeriodicsMode periodics_mode{
      testsuite::DumpControl::PeriodicsMode::kEnabled};
};

class DumperFixture : public ::testing::Test {
 protected:
  DumperFixture() : DumperFixture(DumperFixtureConfig{}) {}

  explicit DumperFixture(DumperFixtureConfig config)
      : root_(fs::blocking::TempDirectory::Create()),
        config_(dump::ConfigFromYaml(kConfig, root_, DummyEntity::kName)),
        control_(config.periodics_mode) {}

  dump::Dumper MakeDumper() {
    return dump::Dumper{
        config_,
        dump::CreateDefaultOperationsFactory(config_),
        engine::current_task::GetTaskProcessor(),
        config_storage_.GetSource(),
        statistics_storage_,
        control_,
        dumpable_,
    };
  }

  const fs::blocking::TempDirectory& GetRoot() const { return root_; }
  const dump::Config& GetConfig() const { return config_; }
  testsuite::DumpControl& GetDumpControl() { return control_; }
  DummyEntity& GetDumpable() { return dumpable_; }

 private:
  fs::blocking::TempDirectory root_;
  dump::Config config_;
  testsuite::DumpControl control_;
  utils::statistics::Storage statistics_storage_;
  dynamic_config::StorageMock config_storage_{{dump::kConfigSet, {}}};
  DummyEntity dumpable_;
};

dump::TimePoint Now() {
  return std::chrono::time_point_cast<dump::TimePoint::duration>(
      utils::datetime::Now());
}

}  // namespace

UTEST_F(DumperFixture, MultipleBumps) {
  auto dumper = MakeDumper();
  dumper.ReadDump();
  utils::datetime::MockNowSet({});
  EXPECT_EQ(GetDumpable().write_count, 0);

  dumper.OnUpdateCompleted(Now(), dump::UpdateType::kModified);
  dumper.WriteDumpSyncDebug();
  EXPECT_EQ(GetDumpable().write_count, 1);

  for (int i = 0; i < 10; ++i) {
    utils::datetime::MockSleep(1s);
    dumper.OnUpdateCompleted(Now(), dump::UpdateType::kAlreadyUpToDate);
    dumper.WriteDumpSyncDebug();

    // No actual updates have been performed, dumper should just rename files
    EXPECT_EQ(GetDumpable().write_count, 1);
  }
}

namespace {
constexpr std::size_t kUpdatersCount = 2;
constexpr std::size_t kWritersCount = 2;
constexpr std::size_t kReadersCount = 2;
constexpr std::size_t kWritersSyncCount = 1;
}  // namespace

UTEST_F_MT(DumperFixture, ThreadSafety,
           kUpdatersCount + kWritersCount + kReadersCount + kWritersSyncCount) {
  std::atomic now{Now()};
  const auto get_now = [&] {
    return utils::AtomicUpdate(now, [](auto old) { return old + 1us; });
  };

  auto dumper = MakeDumper();
  dumper.ReadDump();
  dumper.OnUpdateCompleted(get_now(), dump::UpdateType::kModified);
  dumper.WriteDumpSyncDebug();

  std::atomic<bool> keep_running{true};
  std::vector<engine::TaskWithResult<void>> tasks;

  for (std::size_t i = 0; i < kUpdatersCount; ++i) {
    tasks.push_back(utils::Async("updater", [&, i] {
      while (keep_running) {
        dumper.OnUpdateCompleted(get_now(),
                                 i == 0 ? dump::UpdateType::kModified
                                        : dump::UpdateType::kAlreadyUpToDate);
        engine::Yield();
      }
    }));
  }

  for (std::size_t i = 0; i < kReadersCount; ++i) {
    tasks.push_back(utils::Async("reader", [&dumper, &keep_running] {
      while (keep_running) {
        dumper.ReadDumpDebug();
        engine::Yield();
      }
    }));
  }

  for (int i = 0; i < 100; ++i) {
    dumper.WriteDumpSyncDebug();
  }

  keep_running = false;
  engine::GetAll(tasks);
  dumper.CancelWriteTaskAndWait();

  // Inside 'DummyEntity', there is an 'ASSERT_TRUE' that fires if a data
  // race is detected. The test passes if no data races have been
  // detected.
}

UTEST_F(DumperFixture, OnUpdateCompletedIsAsync) {
  auto dumper = MakeDumper();
  utils::datetime::MockNowSet({});

  {
    std::lock_guard lock(GetDumpable().write_mutex);

    // Async write operation will wait for 'write_mutex', but the method
    // should return instantly
    dumper.OnUpdateCompleted();

    // Allow the asynchronous write operation to start
    // (we abuse the single-threaded-ness of this test)
    engine::Yield();

    utils::datetime::MockSleep(1s);

    // This update should be accounted for, but the dump should not be written
    // immediately, because a previous write is in progress
    dumper.OnUpdateCompleted();
  }

  // The asynchronous write operation will finish, then Dumper should start
  // writing a second dump, which it will be able to do freely.
  engine::Yield();

  EXPECT_EQ(GetDumpable().write_count, 2);
}

UTEST_F(DumperFixture, DontWriteBackTheDumpAfterReading) {
  dump::CreateDump(dump::ToBinary(42), GetConfig());

  auto dumper = MakeDumper();
  utils::datetime::MockNowSet({});

  // The prepared dump should be loaded into 'dumpable_'
  dumper.ReadDumpDebug();
  ASSERT_EQ(GetDumpable().value, 42);

  // Note: no OnUpdateCompleted call. Dumper doesn't know that the update has
  // happened and shouldn't write any dumps.
  GetDumpable().value = 34;

  // No dumps should be written here, because we've read the data from a dump,
  // and there have been no updates since then. (At least Dumper doesn't know
  // of any.)
  dumper.WriteDumpSyncDebug();
  EXPECT_EQ(GetDumpable().write_count, 0);
}

UTEST_F(DumperFixture, UpdateTimeSimple) {
  auto dumper = MakeDumper();
  utils::datetime::MockNowSet({});

  // The asynchronous write operation will not start just yet
  // (we abuse the single-threaded-ness of this test)
  dumper.OnUpdateCompleted();

  utils::datetime::MockSleep(3s);
  const auto write_time = utils::datetime::Now();
  // The asynchronous write will be performed here
  engine::Yield();

  EXPECT_EQ(dumper.ReadDump(), write_time);
}

UTEST_F(DumperFixture, UpdateTimeDetailed) {
  auto dumper = MakeDumper();
  utils::datetime::MockNowSet({});

  const auto explicit_time =
      std::chrono::time_point_cast<dump::TimePoint::duration>(
          utils::datetime::Now());
  // The asynchronous write operation will not start just yet
  // (we abuse the single-threaded-ness of this test)
  dumper.OnUpdateCompleted(explicit_time, dump::UpdateType::kModified);

  utils::datetime::MockSleep(3s);
  // The asynchronous write will be performed here
  engine::Yield();

  EXPECT_EQ(dumper.ReadDump(), explicit_time);
}

UTEST_F(DumperFixture, ReadDumpChecking) {
  auto dumper = MakeDumper();
  UEXPECT_THROW_MSG(
      dumper.WriteDumpSyncDebug(), dump::Error,
      dumper.Name() +
          ": unable to write a dump, there was no attempt to read а dump");
}

namespace {

class DumperFixtureNonPeriodic : public DumperFixture {
 protected:
  DumperFixtureNonPeriodic()
      : DumperFixture([] {
          DumperFixtureConfig config;
          config.periodics_mode =
              testsuite::DumpControl::PeriodicsMode::kDisabled;
          return config;
        }()) {}
};

}  // namespace

UTEST_F(DumperFixtureNonPeriodic, NormalWritesDisabled) {
  auto dumper = MakeDumper();

  for (int i = 0; i < 10; ++i) {
    dumper.OnUpdateCompleted();

    // An asynchronous write would be performed here if periodic dump writes
    // were enabled.
    engine::Yield();

    EXPECT_EQ(GetDumpable().read_count, 0);
    EXPECT_EQ(GetDumpable().write_count, 0);
  }
}

UTEST_F(DumperFixtureNonPeriodic, ReadDumpChecking) {
  auto dumper = MakeDumper();
  UEXPECT_THROW_MSG(
      dumper.WriteDumpSyncDebug(), dump::Error,
      dumper.Name() +
          ": unable to write a dump, there was no attempt to read а dump");
}

// This should not be important for any purpose. The test just documents the
// current behavior.
UTEST_F(DumperFixtureNonPeriodic, NormalReadsEnabled) {
  dump::CreateDump(dump::ToBinary(42), GetConfig());
  auto dumper = MakeDumper();
  EXPECT_NE(dumper.ReadDump(), std::nullopt);
  EXPECT_EQ(GetDumpable().read_count, 1);
  EXPECT_EQ(GetDumpable().write_count, 0);
}

UTEST_F(DumperFixtureNonPeriodic, ForcedWritesEnabled) {
  auto dumper = MakeDumper();
  dumper.ReadDump();
  dumper.OnUpdateCompleted();
  GetDumpControl().WriteCacheDumps({dumper.Name()});
  EXPECT_EQ(GetDumpable().read_count, 0);
  EXPECT_EQ(GetDumpable().write_count, 1);
}

UTEST_F(DumperFixtureNonPeriodic, ForcedReadsEnabled) {
  dump::CreateDump(dump::ToBinary(42), GetConfig());
  auto dumper = MakeDumper();
  GetDumpControl().ReadCacheDumps({dumper.Name()});
  EXPECT_EQ(GetDumpable().read_count, 1);
  EXPECT_EQ(GetDumpable().write_count, 0);
}

namespace {

/// [Sample Dumper usage]
// NOLINTNEXTLINE(fuchsia-multiple-inheritance)
class SampleComponentWithDumps final : public components::LoggableComponentBase,
                                       private dump::DumpableEntity {
 public:
  static constexpr auto kName = "component-with-dumps";

  SampleComponentWithDumps(const components::ComponentConfig& config,
                           const components::ComponentContext& context)
      : LoggableComponentBase(config, context),
        dumper_(config, context, *this) {
    dumper_.ReadDump();
  }

  ~SampleComponentWithDumps() override {
    // This call is necessary in destructor. Otherwise, we could be writing data
    // while the component is being destroyed.
    dumper_.CancelWriteTaskAndWait();
  }

  std::string Get(const std::string& key) {
    if (auto existing_value = data_.Get(key)) return *existing_value;

    auto value = key + "foo";
    data_.Emplace(key, value);

    // Writes a new dump if enough time has passed since the last one
    dumper_.OnUpdateCompleted();
    return value;
  }

 private:
  void GetAndWrite(dump::Writer& writer) const override {
    writer.Write(data_.GetSnapshot());
  }

  void ReadAndSet(dump::Reader& reader) override {
    using Map = std::unordered_map<std::string, std::shared_ptr<std::string>>;
    data_.Assign(reader.Read<Map>());
  }

  rcu::RcuMap<std::string, std::string> data_;
  dump::Dumper dumper_;
};
/// [Sample Dumper usage]

// Can't test the component normally outside the component system :(
[[maybe_unused]] auto UsageSample(const components::ComponentContext& context) {
  return context.FindComponent<SampleComponentWithDumps>().Get("foo");
}

}  // namespace

USERVER_NAMESPACE_END
