#include <cache/dumper.hpp>

#include <set>

#include <cache/cache_test_helpers.hpp>
#include <utest/utest.hpp>

namespace {

cache::TimePoint BaseTime() {
  return std::chrono::time_point_cast<cache::TimePoint::duration>(
      utils::datetime::Stringtime("2015-03-22T09:00:00.000000", "UTC",
                                  cache::kDumpFilenameDateFormat));
}

std::vector<boost::filesystem::path> InitialFileNames() {
  return {
      "2015-03-22T09:00:00.000000-v5",  "2015-03-22T09:00:00.000000-v0",
      "2015-03-22T09:00:00.000000-v42", "2015-03-22T09:00:01.000000-v5",
      "2015-03-22T09:00:02.000000-v5",  "2015-03-22T09:00:03.000000-v5",
  };
}

std::vector<boost::filesystem::path> JunkFileNames() {
  return {
      "2015-03-22T09:00:00.000000-v0.tmp",
      "2015-03-22T09:00:00.000000-v5.tmp",
      "2000-01-01T00:00:00.000000-v42.tmp",
  };
}

std::vector<boost::filesystem::path> UnrelatedFileNames() {
  return {"blah-2015-03-22T09:00:00.000000-v5",
          "blah-2015-03-22T09:00:00.000000-v5.tmp",
          "foo",
          "foo.tmp",
          "2015-03-22T09:00:00.000000-v-5",
          "2015-03-22T09:00:00.000000-v-5.tmp",
          "2015-03-22T09:00:00.000000-5.tmp"};
}

template <typename Container, typename Range>
void InsertAll(Container& destination, Range&& source) {
  destination.insert(std::begin(source), std::end(source));
}

constexpr std::string_view kCacheName = "name";

}  // namespace

TEST(CacheDumper, CleanupTmpTest) {
  const std::string kConfig = R"(
update-interval: 1s
dump:
    format-version: 5
    max-count: 10
)";
  const auto directory = cache::RandomDumpDirectory();
  utils::ScopeGuard guard([&directory] { cache::ClearDumps(directory); });

  cache::CreateDumps(InitialFileNames(), directory, kCacheName);
  cache::CreateDumps(JunkFileNames(), directory, kCacheName);
  cache::CreateDumps(UnrelatedFileNames(), directory, kCacheName);

  // Expected to remove .tmp junk and a dump with version 0
  std::set<boost::filesystem::path> expected_files;
  InsertAll(expected_files, InitialFileNames());
  InsertAll(expected_files, UnrelatedFileNames());
  ASSERT_TRUE(expected_files.erase("2015-03-22T09:00:00.000000-v0"));

  utils::datetime::MockNowSet(BaseTime());

  RunInCoro([&] {
    cache::Dumper dumper(cache::ConfigFromYaml(kConfig, directory.string(),
                                               std::string{kCacheName}),
                         engine::current_task::GetTaskProcessor(), kCacheName);

    dumper.Cleanup();
  });

  EXPECT_EQ(cache::FilenamesInDirectory(directory, kCacheName), expected_files);
}

TEST(CacheDumper, CleanupByAgeTest) {
  const std::string kConfig = R"(
update-interval: 1s
dump:
    format-version: 5
    max-count: 10
    max-age: 1500ms
)";
  const auto directory = cache::RandomDumpDirectory();
  utils::ScopeGuard guard([&directory] { cache::ClearDumps(directory); });

  cache::CreateDumps(InitialFileNames(), directory, kCacheName);
  cache::CreateDumps(JunkFileNames(), directory, kCacheName);
  cache::CreateDumps(UnrelatedFileNames(), directory, kCacheName);

  // Expected to remove .tmp junk, a dump with version 0, and old dumps
  std::set<boost::filesystem::path> expected_files;
  InsertAll(expected_files, InitialFileNames());
  InsertAll(expected_files, UnrelatedFileNames());
  ASSERT_TRUE(expected_files.erase("2015-03-22T09:00:00.000000-v0"));
  ASSERT_TRUE(expected_files.erase("2015-03-22T09:00:00.000000-v5"));
  ASSERT_TRUE(expected_files.erase("2015-03-22T09:00:01.000000-v5"));
  ASSERT_TRUE(expected_files.erase("2015-03-22T09:00:00.000000-v42"));

  utils::datetime::MockNowSet(BaseTime());
  utils::datetime::MockSleep(std::chrono::seconds{3});

  RunInCoro([&] {
    cache::Dumper dumper(cache::ConfigFromYaml(kConfig, directory.string(),
                                               std::string{kCacheName}),
                         engine::current_task::GetTaskProcessor(), kCacheName);

    dumper.Cleanup();
  });

  EXPECT_EQ(cache::FilenamesInDirectory(directory, kCacheName), expected_files);
}

TEST(CacheDumper, CleanupByCountTest) {
  const std::string kConfig = R"(
update-interval: 1s
dump:
    format-version: 5
    max-count: 1
)";
  const auto directory = cache::RandomDumpDirectory();
  utils::ScopeGuard guard([&directory] { cache::ClearDumps(directory); });

  cache::CreateDumps(InitialFileNames(), directory, kCacheName);
  cache::CreateDumps(JunkFileNames(), directory, kCacheName);
  cache::CreateDumps(UnrelatedFileNames(), directory, kCacheName);

  // Expected to remove .tmp junk, a dump with version 0, and some current dumps
  std::set<boost::filesystem::path> expected_files;
  InsertAll(expected_files, InitialFileNames());
  InsertAll(expected_files, UnrelatedFileNames());
  ASSERT_TRUE(expected_files.erase("2015-03-22T09:00:00.000000-v0"));
  ASSERT_TRUE(expected_files.erase("2015-03-22T09:00:00.000000-v5"));
  ASSERT_TRUE(expected_files.erase("2015-03-22T09:00:01.000000-v5"));
  ASSERT_TRUE(expected_files.erase("2015-03-22T09:00:02.000000-v5"));

  RunInCoro([&] {
    cache::Dumper dumper(cache::ConfigFromYaml(kConfig, directory.string(),
                                               std::string{kCacheName}),
                         engine::current_task::GetTaskProcessor(), kCacheName);

    dumper.Cleanup();
  });

  EXPECT_EQ(cache::FilenamesInDirectory(directory, kCacheName), expected_files);
}

TEST(CacheDumper, ReadLatestDumpTest) {
  const std::string kConfig = R"(
update-interval: 1s
dump:
    format-version: 5
)";
  const auto directory = cache::RandomDumpDirectory();
  utils::ScopeGuard guard([&directory] { cache::ClearDumps(directory); });

  cache::CreateDumps(InitialFileNames(), directory, kCacheName);
  cache::CreateDumps(JunkFileNames(), directory, kCacheName);
  cache::CreateDumps(UnrelatedFileNames(), directory, kCacheName);

  // Expected not to write or remove anything
  std::set<boost::filesystem::path> expected_files;
  InsertAll(expected_files, InitialFileNames());
  InsertAll(expected_files, JunkFileNames());
  InsertAll(expected_files, UnrelatedFileNames());

  RunInCoro([&] {
    cache::Dumper dumper(cache::ConfigFromYaml(kConfig, directory.string(),
                                               std::string{kCacheName}),
                         engine::current_task::GetTaskProcessor(), kCacheName);

    const auto dump = dumper.ReadLatestDump();
    EXPECT_TRUE(dump);

    if (dump) {
      using namespace std::chrono_literals;
      EXPECT_EQ(dump->contents, "2015-03-22T09:00:03.000000-v5");
      EXPECT_EQ(dump->update_time, BaseTime() + 3s);
    }
  });

  EXPECT_EQ(cache::FilenamesInDirectory(directory, kCacheName), expected_files);
}

TEST(CacheDumper, DumpAndBumpTest) {
  const std::string kConfig = R"(
update-interval: 1s
dump:
    format-version: 5
)";
  const auto directory = cache::RandomDumpDirectory();
  utils::ScopeGuard guard([&directory] { cache::ClearDumps(directory); });

  cache::CreateDumps(InitialFileNames(), directory, kCacheName);
  cache::CreateDumps(JunkFileNames(), directory, kCacheName);
  cache::CreateDumps(UnrelatedFileNames(), directory, kCacheName);

  // Expected to write a new dump
  std::set<boost::filesystem::path> expected_files;
  InsertAll(expected_files, InitialFileNames());
  InsertAll(expected_files, JunkFileNames());
  InsertAll(expected_files, UnrelatedFileNames());
  expected_files.insert("2015-03-22T09:00:08.000000-v5");

  RunInCoro([&] {
    using namespace std::chrono_literals;

    cache::Dumper dumper(cache::ConfigFromYaml(kConfig, directory.string(),
                                               std::string{kCacheName}),
                         engine::current_task::GetTaskProcessor(), kCacheName);

    const auto written_dump = cache::DumpContents{"abc", BaseTime() + 5s};
    EXPECT_TRUE(dumper.WriteNewDump(written_dump));

    EXPECT_TRUE(dumper.BumpDumpTime(written_dump.update_time,
                                    written_dump.update_time + 3s));

    const auto retrieved_dump = dumper.ReadLatestDump();
    EXPECT_TRUE(retrieved_dump);
    if (retrieved_dump) {
      EXPECT_EQ(retrieved_dump->contents, written_dump.contents);
      EXPECT_EQ(retrieved_dump->update_time, written_dump.update_time + 3s);
    }
  });

  EXPECT_EQ(cache::FilenamesInDirectory(directory, kCacheName), expected_files);
}
