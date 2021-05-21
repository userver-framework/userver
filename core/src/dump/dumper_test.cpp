#include <dump/dumper.hpp>

#include <chrono>

#include <boost/filesystem.hpp>

#include <dump/common.hpp>
#include <dump/factory.hpp>
#include <dump/internal_test_helpers.hpp>
#include <dump/test_helpers.hpp>
#include <engine/mutex.hpp>
#include <engine/sleep.hpp>
#include <engine/task/task_processor.hpp>
#include <engine/task/task_with_result.hpp>
#include <fs/blocking/temp_directory.hpp>
#include <testsuite/dump_control.hpp>
#include <utest/utest.hpp>
#include <utils/async.hpp>
#include <utils/mock_now.hpp>

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
  void SetUp() override {
    root_ = fs::blocking::TempDirectory::Create();
    config_.emplace(dump::ConfigFromYaml(kConfig, root_, DummyEntity::kName));
    dumpable_.emplace();
  }

  dump::Dumper MakeDumper() {
    return {*config_, dump::CreateDefaultOperationsFactory(*config_),
            engine::current_task::GetTaskProcessor(), control_, *dumpable_};
  }

  fs::blocking::TempDirectory root_;
  std::optional<dump::Config> config_;
  testsuite::DumpControl control_;
  std::optional<DummyEntity> dumpable_;
};

dump::TimePoint Now() {
  return std::chrono::time_point_cast<dump::TimePoint::duration>(
      utils::datetime::Now());
};

}  // namespace

TEST_F(DumperFixture, MultipleBumps) {
  RunInCoro([this] {
    using namespace std::chrono_literals;

    auto dumper = MakeDumper();
    utils::datetime::MockNowSet({});
    EXPECT_EQ(dumpable_->write_count, 0);

    dumper.OnUpdateCompleted(Now(), true);
    dumper.WriteDumpSyncDebug();
    EXPECT_EQ(dumpable_->write_count, 1);

    for (int i = 0; i < 10; ++i) {
      utils::datetime::MockSleep(1s);
      dumper.OnUpdateCompleted(Now(), false);
      dumper.WriteDumpSyncDebug();

      // No actual updates have been performed, dumper should just rename files
      EXPECT_EQ(dumpable_->write_count, 1);
    }
  });
}

TEST_F(DumperFixture, ThreadSafety) {
  RunInCoro(
      [this] {
        using namespace std::chrono_literals;

        auto dumper = MakeDumper();
        utils::datetime::MockNowSet({});
        dumper.OnUpdateCompleted(Now(), true);
        dumper.WriteDumpSyncDebug();

        std::vector<engine::TaskWithResult<void>> tasks;

        for (int i = 0; i < 2; ++i) {
          tasks.push_back(utils::Async("updater", [&dumper, i] {
            while (!engine::current_task::IsCancelRequested()) {
              dumper.OnUpdateCompleted(Now(), i == 1);
              utils::datetime::MockSleep(1s);
              engine::Yield();
            }
          }));

          tasks.push_back(utils::Async("writer", [&dumper] {
            while (!engine::current_task::IsCancelRequested()) {
              dumper.WriteDumpAsync();
              engine::Yield();
            }
          }));

          tasks.push_back(utils::Async("reader", [&dumper] {
            while (!engine::current_task::IsCancelRequested()) {
              dumper.ReadDumpDebug();
              engine::Yield();
            }
          }));
        }

        for (int i = 0; i < 100; ++i) {
          dumper.WriteDumpSyncDebug();
        }

        for (auto& task : tasks) {
          task.SyncCancel();
        }
        dumper.CancelWriteTaskAndWait();

        // If no ASSERT_TRUE's has failed inside DummyEntity, the test passes
      },
      std::thread::hardware_concurrency());
}

TEST_F(DumperFixture, WriteDumpAsyncIsAsync) {
  RunInCoro([this] {
    using namespace std::chrono_literals;

    auto dumper = MakeDumper();
    utils::datetime::MockNowSet({});
    dumper.OnUpdateCompleted(Now(), true);

    {
      std::lock_guard lock(dumpable_->write_mutex);

      // Async write operation will wait for 'write_mutex', but the method
      // should return instantly
      dumper.WriteDumpAsync();

      utils::datetime::MockSleep(1s);
      dumper.OnUpdateCompleted(Now(), true);

      // This write should be dropped, because a previous write is in progress
      dumper.WriteDumpAsync();
    }

    // 'WriteDumpSyncDebug' will wait until the first write completes
    utils::datetime::MockSleep(1s);
    dumper.OnUpdateCompleted(Now(), true);
    dumper.WriteDumpSyncDebug();

    EXPECT_EQ(dumpable_->write_count, 2);
  });
}

TEST_F(DumperFixture, DontWriteBackTheDumpAfterReading) {
  RunInCoro([this] {
    dump::CreateDump(dump::ToBinary(42), *config_);

    auto dumper = MakeDumper();
    utils::datetime::MockNowSet({});

    // The prepared dump should be loaded into 'dumpable_'
    dumper.ReadDumpDebug();
    ASSERT_EQ(dumpable_->value, 42);

    // Note: no OnUpdateCompleted call. Dumper doesn't know that the dump has
    // happened and shouldn't write any dumps.
    dumpable_->value = 34;

    // No dumps should be written here, because we've read the data from a dump,
    // and there have been no updates since then. (At least Dumper doesn't know
    // of any.)
    dumper.WriteDumpSyncDebug();
    EXPECT_EQ(dumpable_->write_count, 0);
  });
}
