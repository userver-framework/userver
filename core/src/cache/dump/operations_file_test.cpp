#include <cache/dump/operations_file.hpp>

#include <fs/blocking/read.hpp>
#include <fs/blocking/temp_directory.hpp>
#include <fs/blocking/write.hpp>
#include <utest/utest.hpp>

namespace {

std::string DumpFilePath(fs::blocking::TempDirectory& dir) {
  return dir.GetPath() + "/cache-dump";
}

}  // namespace

TEST(CacheDumpOperationsFile, WriteReadRaw) {
  fs::blocking::TempDirectory dir;
  const auto path = DumpFilePath(dir);

  constexpr std::size_t kMaxLength = 10;
  std::size_t total_length = 0;

  cache::dump::FileWriter writer(path);
  for (std::size_t i = 0; i <= kMaxLength; ++i) {
    writer.WriteRaw(std::string(i, 'a'));
    total_length += i;
  }
  writer.Finish();

  EXPECT_EQ(fs::blocking::ReadFileContents(path),
            std::string(total_length, 'a'));

  cache::dump::FileReader reader(path);
  for (std::size_t i = 0; i <= kMaxLength; ++i) {
    EXPECT_EQ(reader.ReadRaw(i), std::string(i, 'a'));
  }
  reader.Finish();
}

TEST(CacheDumpOperationsFile, EmptyDump) {
  fs::blocking::TempDirectory dir;
  const auto path = DumpFilePath(dir);

  cache::dump::FileWriter writer(path);
  writer.Finish();

  EXPECT_EQ(fs::blocking::ReadFileContents(path), "");

  cache::dump::FileReader reader(path);
  reader.Finish();
}

TEST(CacheDumpOperationsFile, EmptyStringDump) {
  fs::blocking::TempDirectory dir;
  const auto path = DumpFilePath(dir);

  cache::dump::FileWriter writer(path);
  writer.WriteRaw({});
  writer.Finish();

  EXPECT_EQ(fs::blocking::ReadFileContents(path), "");

  cache::dump::FileReader reader(path);
  EXPECT_EQ(reader.ReadRaw(0), "");
  reader.Finish();
}

TEST(CacheDumpOperationsFile, Overread) {
  fs::blocking::TempDirectory dir;
  const auto path = DumpFilePath(dir);

  fs::blocking::RewriteFileContents(path, std::string(10, 'a'));

  cache::dump::FileReader reader(path);
  EXPECT_THROW(reader.ReadRaw(11), cache::dump::Error);
}

TEST(CacheDumpOperationsFile, Underread) {
  fs::blocking::TempDirectory dir;
  const auto path = DumpFilePath(dir);

  fs::blocking::RewriteFileContents(path, std::string(10, 'a'));

  cache::dump::FileReader reader(path);
  EXPECT_EQ(reader.ReadRaw(9), std::string(9, 'a'));
  EXPECT_THROW(reader.Finish(), cache::dump::Error);
}
