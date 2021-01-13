#include <cache/dump/operations_file.hpp>

#include <boost/regex.hpp>

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

  cache::dump::FileWriter writer(path, boost::filesystem::perms::owner_read);
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

  cache::dump::FileWriter writer(path, boost::filesystem::perms::owner_read);
  writer.Finish();

  EXPECT_EQ(fs::blocking::ReadFileContents(path), "");

  cache::dump::FileReader reader(path);
  reader.Finish();
}

TEST(CacheDumpOperationsFile, EmptyStringDump) {
  fs::blocking::TempDirectory dir;
  const auto path = DumpFilePath(dir);

  cache::dump::FileWriter writer(path, boost::filesystem::perms::owner_read);
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
  try {
    reader.ReadRaw(11);
  } catch (const cache::dump::Error& ex) {
    EXPECT_TRUE(boost::regex_match(
        ex.what(),
        boost::regex{
            "Unexpected end-of-file while trying to read from the dump file "
            "\".+\": file-size=10, position=0, requested-size=11"}))
        << ex.what();
    return;
  }
  FAIL();
}

TEST(CacheDumpOperationsFile, Underread) {
  fs::blocking::TempDirectory dir;
  const auto path = DumpFilePath(dir);

  fs::blocking::RewriteFileContents(path, std::string(10, 'a'));

  cache::dump::FileReader reader(path);
  EXPECT_EQ(reader.ReadRaw(9), std::string(9, 'a'));
  try {
    reader.Finish();
  } catch (const cache::dump::Error& ex) {
    EXPECT_TRUE(boost::regex_match(
        ex.what(),
        boost::regex{"Unexpected extra data at the end of the dump file "
                     "\".+\": file-size=10, position=9, unread-size=1"}))
        << ex.what();
    return;
  }
  FAIL();
}
