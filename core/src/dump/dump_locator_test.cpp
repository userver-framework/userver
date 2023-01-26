#include <dump/dump_locator.hpp>

#include <set>

#include <dump/internal_helpers_test.hpp>
#include <userver/fs/blocking/read.hpp>
#include <userver/fs/blocking/temp_directory.hpp>
#include <userver/fs/blocking/write.hpp>
#include <userver/utest/utest.hpp>
#include <userver/utils/mock_now.hpp>

USERVER_NAMESPACE_BEGIN

namespace {

dump::TimePoint BaseTime() {
  return std::chrono::time_point_cast<dump::TimePoint::duration>(
      utils::datetime::Stringtime("2015-03-22T090000.000000Z", "UTC",
                                  dump::kFilenameDateFormat));
}

std::vector<std::string> InitialFileNames() {
  return {
      "2015-03-22T090000.000000Z-v5",  "2015-03-22T090000.000000Z-v0",
      "2015-03-22T090000.000000Z-v42", "2015-03-22T090001.000000Z-v5",
      "2015-03-22T090002.000000Z-v5",  "2015-03-22T090003.000000Z-v5",
  };
}

std::vector<std::string> JunkFileNames() {
  return {
      "2015-03-22T090000.000000Z-v0.tmp",
      "2015-03-22T090000.000000Z-v5.tmp",
      "2000-01-01T000000.000000Z-v42.tmp",
  };
}

std::vector<std::string> UnrelatedFileNames() {
  return {"blah-2015-03-22T090000.000000Z-v5",
          "blah-2015-03-22T090000.000000Z-v5.tmp",
          "foo",
          "foo.tmp",
          "2015-03-22T090000.000000Z-v-5",
          "2015-03-22T090000.000000Z-v-5.tmp",
          "2015-03-22T090000.000000Z-5.tmp"};
}

template <typename Container, typename Range>
void InsertAll(Container& destination, Range&& source) {
  destination.insert(std::begin(source), std::end(source));
}

std::string Filename(const std::string& full_path) {
  return boost::filesystem::path{full_path}.filename().string();
}

constexpr std::string_view kDumperName = "name";

}  // namespace

UTEST(DumpLocator, CleanupTmp) {
  const std::string kConfig = R"(
enable: true
world-readable: false
format-version: 5
max-count: 10
max-age: null
)";
  const auto dir = fs::blocking::TempDirectory::Create();

  dump::CreateDumps(InitialFileNames(), dir, kDumperName);
  dump::CreateDumps(JunkFileNames(), dir, kDumperName);
  dump::CreateDumps(UnrelatedFileNames(), dir, kDumperName);

  // Expected to remove .tmp junk and a dump with version 0
  std::set<std::string> expected_files;
  InsertAll(expected_files, InitialFileNames());
  InsertAll(expected_files, UnrelatedFileNames());
  ASSERT_TRUE(expected_files.erase("2015-03-22T090000.000000Z-v0"));

  utils::datetime::MockNowSet(BaseTime());

  const dump::Config config{dump::ConfigFromYaml(kConfig, dir, kDumperName)};
  dump::DumpLocator locator{config};
  locator.Cleanup();

  EXPECT_EQ(dump::FilenamesInDirectory(dir, kDumperName), expected_files);
}

UTEST(DumpLocator, CleanupByAge) {
  const std::string kConfig = R"(
enable: true
world-readable: false
format-version: 5
max-count: 10
max-age: 1500ms
)";
  const auto dir = fs::blocking::TempDirectory::Create();

  dump::CreateDumps(InitialFileNames(), dir, kDumperName);
  dump::CreateDumps(JunkFileNames(), dir, kDumperName);
  dump::CreateDumps(UnrelatedFileNames(), dir, kDumperName);

  // Expected to remove .tmp junk, a dump with version 0, and old dumps
  std::set<std::string> expected_files;
  InsertAll(expected_files, InitialFileNames());
  InsertAll(expected_files, UnrelatedFileNames());
  ASSERT_TRUE(expected_files.erase("2015-03-22T090000.000000Z-v0"));
  ASSERT_TRUE(expected_files.erase("2015-03-22T090000.000000Z-v5"));
  ASSERT_TRUE(expected_files.erase("2015-03-22T090001.000000Z-v5"));
  ASSERT_TRUE(expected_files.erase("2015-03-22T090000.000000Z-v42"));

  utils::datetime::MockNowSet(BaseTime());
  utils::datetime::MockSleep(std::chrono::seconds{3});

  const dump::Config config{dump::ConfigFromYaml(kConfig, dir, kDumperName)};
  dump::DumpLocator locator{config};
  locator.Cleanup();

  EXPECT_EQ(dump::FilenamesInDirectory(dir, kDumperName), expected_files);
}

UTEST(DumpLocator, CleanupByCount) {
  const std::string kConfig = R"(
enable: true
world-readable: false
format-version: 5
max-count: 1
max-age: null
)";
  const auto dir = fs::blocking::TempDirectory::Create();

  dump::CreateDumps(InitialFileNames(), dir, kDumperName);
  dump::CreateDumps(JunkFileNames(), dir, kDumperName);
  dump::CreateDumps(UnrelatedFileNames(), dir, kDumperName);

  // Expected to remove .tmp junk, a dump with version 0, and some current dumps
  std::set<std::string> expected_files;
  InsertAll(expected_files, InitialFileNames());
  InsertAll(expected_files, UnrelatedFileNames());
  ASSERT_TRUE(expected_files.erase("2015-03-22T090000.000000Z-v0"));
  ASSERT_TRUE(expected_files.erase("2015-03-22T090000.000000Z-v5"));
  ASSERT_TRUE(expected_files.erase("2015-03-22T090001.000000Z-v5"));
  ASSERT_TRUE(expected_files.erase("2015-03-22T090002.000000Z-v5"));

  const dump::Config config{dump::ConfigFromYaml(kConfig, dir, kDumperName)};
  dump::DumpLocator locator{config};
  locator.Cleanup();

  EXPECT_EQ(dump::FilenamesInDirectory(dir, kDumperName), expected_files);
}

UTEST(DumpLocator, ReadLatestDump) {
  const std::string kConfig = R"(
enable: true
world-readable: false
format-version: 5
max-age: null
)";
  const auto dir = fs::blocking::TempDirectory::Create();

  dump::CreateDumps(InitialFileNames(), dir, kDumperName);
  dump::CreateDumps(JunkFileNames(), dir, kDumperName);
  dump::CreateDumps(UnrelatedFileNames(), dir, kDumperName);

  // Expected not to write or remove anything
  std::set<std::string> expected_files;
  InsertAll(expected_files, InitialFileNames());
  InsertAll(expected_files, JunkFileNames());
  InsertAll(expected_files, UnrelatedFileNames());

  const dump::Config config{dump::ConfigFromYaml(kConfig, dir, kDumperName)};
  dump::DumpLocator locator{config};

  auto dump_info = locator.GetLatestDump();
  EXPECT_TRUE(dump_info);

  if (dump_info) {
    using namespace std::chrono_literals;
    EXPECT_EQ(dump_info->update_time, BaseTime() + 3s);
    EXPECT_EQ(fs::blocking::ReadFileContents(dump_info->full_path),
              "2015-03-22T090003.000000Z-v5");
  }

  EXPECT_EQ(dump::FilenamesInDirectory(dir, kDumperName), expected_files);
}

UTEST(DumpLocator, DumpAndBump) {
  const std::string kConfig = R"(
enable: true
world-readable: false
format-version: 5
max-age: null
)";
  const auto dir = fs::blocking::TempDirectory::Create();

  // Expected to get a new dump with update_time = BaseTime() + 3s
  std::set<std::string> expected_files;
  expected_files.insert("2015-03-22T090003.000000Z-v5");

  using namespace std::chrono_literals;

  const dump::Config config{dump::ConfigFromYaml(kConfig, dir, kDumperName)};
  dump::DumpLocator locator{config};

  auto old_update_time = BaseTime();
  auto dump_stats = locator.RegisterNewDump(old_update_time);

  fs::blocking::RewriteFileContents(dump_stats.full_path, "abc");

  // Emulate a new update that happened 3s later and got identical data
  auto new_update_time = BaseTime() + 3s;
  EXPECT_TRUE(locator.BumpDumpTime(old_update_time, new_update_time));

  auto dump_info = locator.GetLatestDump();
  ASSERT_TRUE(dump_info);

  EXPECT_EQ(fs::blocking::ReadFileContents(dump_info->full_path), "abc");
  EXPECT_EQ(dump_info->update_time, new_update_time);

  EXPECT_EQ(dump::FilenamesInDirectory(dir, kDumperName), expected_files);
}

UTEST(DumpLocator, LegacyFilenames) {
  using namespace std::chrono_literals;
  using namespace std::string_literals;

  const std::string kConfig = R"(
enable: true
world-readable: false
format-version: 5
max-count: 5
max-age: 30m
)";
  const auto dir = fs::blocking::TempDirectory::Create();

  const std::string file1 = "2015-03-22T09:00:00.000000-v5"s;
  const std::string file2 = "2015-03-22T091000.000000Z-v5"s;
  const std::string file3 = "2015-03-22T094000.000000Z-v5"s;
  const std::string file4 = "2015-03-22T09:50:00.000000-v5";
  dump::CreateDumps({file1, file2, file3, file4}, dir, kDumperName);

  utils::datetime::MockNowSet(BaseTime() + 60min);  // 2015-03-22T100000.000000Z

  const dump::Config config{dump::ConfigFromYaml(kConfig, dir, kDumperName)};
  dump::DumpLocator locator{config};

  {
    // A legacy-filename dump should be discovered successfully
    const auto dump_stats = locator.GetLatestDump();
    ASSERT_TRUE(dump_stats);
    EXPECT_EQ(Filename(dump_stats->full_path), file4);
    EXPECT_EQ(dump_stats->update_time, BaseTime() + 50min);
  }

  {
    // Cleanup should work with normal and legacy-filename dumps correctly
    locator.Cleanup();
    EXPECT_EQ(dump::FilenamesInDirectory(dir, kDumperName),
              (std::set<std::string>{file3, file4}));
  }

  {
    // Update time bumps from legacy filenames is not supported
    const auto dump_stats =
        locator.BumpDumpTime(BaseTime() + 50min, BaseTime() + 60min);
    EXPECT_FALSE(dump_stats);
  }
}

USERVER_NAMESPACE_END
