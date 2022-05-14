#include <userver/dump/dumper.hpp>

#include <atomic>
#include <chrono>
#include <unordered_map>
#include <vector>

#include <fmt/format.h>
#include <fmt/ranges.h>
#include <boost/lockfree/queue.hpp>

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
#include <userver/utest/utest.hpp>
#include <userver/utils/async.hpp>
#include <userver/utils/atomic.hpp>
#include <userver/utils/mock_now.hpp>
#include <userver/utils/statistics/storage.hpp>

#include <dump/internal_helpers_test.hpp>

USERVER_NAMESPACE_BEGIN

namespace {

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

class DumperFixture : public ::testing::Test {
 protected:
  DumperFixture()
      : root_(fs::blocking::TempDirectory::Create()),
        config_(dump::ConfigFromYaml(kConfig, root_, DummyEntity::kName)) {}

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
  using namespace std::chrono_literals;

  auto dumper = MakeDumper();
  utils::datetime::MockNowSet({});
  EXPECT_EQ(dumpable_.write_count, 0);

  dumper.OnUpdateCompleted(Now(), dump::UpdateType::kModified);
  dumper.WriteDumpSyncDebug();
  EXPECT_EQ(dumpable_.write_count, 1);

  for (int i = 0; i < 10; ++i) {
    utils::datetime::MockSleep(1s);
    dumper.OnUpdateCompleted(Now(), dump::UpdateType::kAlreadyUpToDate);
    dumper.WriteDumpSyncDebug();

    // No actual updates have been performed, dumper should just rename files
    EXPECT_EQ(dumpable_.write_count, 1);
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
  using namespace std::chrono_literals;

  std::atomic now{Now()};
  const auto get_now = [&] {
    return utils::AtomicUpdate(now, [](auto old) { return old + 1us; });
  };

  boost::lockfree::queue<const char*> logs(0);

  auto dumper = MakeDumper();
  dumper.OnUpdateCompleted(get_now(), dump::UpdateType::kModified);
  dumper.WriteDumpSyncDebug();

  std::atomic<bool> keep_running{true};
  std::vector<engine::TaskWithResult<void>> tasks;

  for (std::size_t i = 0; i < kUpdatersCount; ++i) {
    tasks.push_back(utils::Async("updater", [&, i] {
      while (keep_running) {
        ASSERT_TRUE(logs.push("OnUpdateCompleted-start"));
        dumper.OnUpdateCompleted(get_now(),
                                 i == 0 ? dump::UpdateType::kModified
                                        : dump::UpdateType::kAlreadyUpToDate);
        ASSERT_TRUE(logs.push("OnUpdateCompleted-stop"));
        engine::Yield();
      }
    }));
  }

  for (std::size_t i = 0; i < kWritersCount; ++i) {
    tasks.push_back(utils::Async("writer", [&dumper, &keep_running, &logs] {
      while (keep_running) {
        ASSERT_TRUE(logs.push("WriteDumpAsync-start"));
        dumper.WriteDumpAsync();
        ASSERT_TRUE(logs.push("WriteDumpAsync-stop"));
        engine::Yield();
      }
    }));
  }

  for (std::size_t i = 0; i < kReadersCount; ++i) {
    tasks.push_back(utils::Async("reader", [&dumper, &keep_running, &logs] {
      while (keep_running) {
        ASSERT_TRUE(logs.push("ReadDumpDebug-start"));
        dumper.ReadDumpDebug();
        ASSERT_TRUE(logs.push("ReadDumpDebug-stop"));
        engine::Yield();
      }
    }));
  }

  for (int i = 0; i < 100; ++i) {
    try {
      dumper.WriteDumpSyncDebug();
    } catch (const std::runtime_error& ex) {
      std::vector<const char*> messages;
      const char* next_message{};
      while (logs.pop(next_message)) {
        messages.push_back(next_message);
      }

      constexpr std::size_t kLastMessagesCount = 100;
      if (messages.size() > kLastMessagesCount) {
        messages.erase(messages.begin(), messages.end() - kLastMessagesCount);
      }

      FAIL() << ex.what() << ". Iteration=" << i << ". Dump filenames="
             << fmt::to_string(
                    dump::FilenamesInDirectory(root_, DummyEntity::kName))
             << ". Operations log=" << fmt::to_string(messages);
      break;
    }
  }

  keep_running = false;
  engine::GetAll(tasks);
  dumper.CancelWriteTaskAndWait();

  // Inside 'DummyEntity', there is an 'ASSERT_TRUE' that fires if a data
  // race is detected. The test passes if no data races have been
  // detected.
}

UTEST_F(DumperFixture, WriteDumpAsyncIsAsync) {
  using namespace std::chrono_literals;

  auto dumper = MakeDumper();
  utils::datetime::MockNowSet({});

  {
    std::lock_guard lock(dumpable_.write_mutex);

    // Async write operation will wait for 'write_mutex', but the method
    // should return instantly
    dumper.SetModifiedAndWriteAsync();
    utils::datetime::MockSleep(1s);

    // This write should be dropped, because a previous write is in progress
    dumper.SetModifiedAndWriteAsync();
  }

  // 'WriteDumpSyncDebug' will wait until the first write completes
  dumper.WriteDumpSyncDebug();

  EXPECT_EQ(dumpable_.write_count, 2);
}

UTEST_F(DumperFixture, DontWriteBackTheDumpAfterReading) {
  dump::CreateDump(dump::ToBinary(42), config_);

  auto dumper = MakeDumper();
  utils::datetime::MockNowSet({});

  // The prepared dump should be loaded into 'dumpable_'
  dumper.ReadDumpDebug();
  ASSERT_EQ(dumpable_.value, 42);

  // Note: no OnUpdateCompleted call. Dumper doesn't know that the update has
  // happened and shouldn't write any dumps.
  dumpable_.value = 34;

  // No dumps should be written here, because we've read the data from a dump,
  // and there have been no updates since then. (At least Dumper doesn't know
  // of any.)
  dumper.WriteDumpSyncDebug();
  EXPECT_EQ(dumpable_.write_count, 0);
}

namespace {

/// [Sample Dumper usage]
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
    // This call is necessary in destructor. Otherwise we could be writing data
    // while the component is being destroyed.
    dumper_.CancelWriteTaskAndWait();
  }

  std::string Get(const std::string& key) {
    if (auto existing_value = data_.Get(key)) return *existing_value;

    auto value = key + "foo";
    data_.Emplace(key, value);

    // Writes a new dump if enough time has passed since the last one
    dumper_.SetModifiedAndWriteAsync();
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
