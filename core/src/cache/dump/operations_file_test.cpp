#include <cache/dump/operations_file.hpp>

#include <boost/regex.hpp>

#include <cache/dump/unsafe.hpp>
#include <fs/blocking/read.hpp>
#include <fs/blocking/temp_directory.hpp>
#include <fs/blocking/temp_file.hpp>
#include <fs/blocking/write.hpp>
#include <tracing/span.hpp>
#include <utest/utest.hpp>

namespace {

std::string DumpFilePath(const fs::blocking::TempDirectory& dir) {
  return dir.GetPath() + "/cache-dump";
}

}  // namespace

TEST(CacheDumpOperationsFile, WriteReadRaw) {
  RunInCoro([] {
    const auto dir = fs::blocking::TempDirectory::Create();
    const auto path = DumpFilePath(dir);

    constexpr std::size_t kMaxLength = 10;
    std::size_t total_length = 0;

    auto scope_time = tracing::Span::CurrentSpan().CreateScopeTime("dump");
    cache::dump::FileWriter writer(path, boost::filesystem::perms::owner_read,
                                   scope_time);
    for (std::size_t i = 0; i <= kMaxLength; ++i) {
      WriteStringViewUnsafe(writer, std::string(i, 'a'));
      total_length += i;
    }
    writer.Finish();

    EXPECT_EQ(fs::blocking::ReadFileContents(path),
              std::string(total_length, 'a'));

    cache::dump::FileReader reader(path);
    for (std::size_t i = 0; i <= kMaxLength; ++i) {
      EXPECT_EQ(ReadStringViewUnsafe(reader, i), std::string(i, 'a'));
    }
    reader.Finish();
  });
}

TEST(CacheDumpOperationsFile, EmptyDump) {
  RunInCoro([] {
    const auto dir = fs::blocking::TempDirectory::Create();
    const auto path = DumpFilePath(dir);

    auto scope_time = tracing::Span::CurrentSpan().CreateScopeTime("dump");
    cache::dump::FileWriter writer(path, boost::filesystem::perms::owner_read,
                                   scope_time);
    writer.Finish();

    EXPECT_EQ(fs::blocking::ReadFileContents(path), "");

    cache::dump::FileReader reader(path);
    reader.Finish();
  });
}

TEST(CacheDumpOperationsFile, EmptyStringDump) {
  RunInCoro([] {
    const auto dir = fs::blocking::TempDirectory::Create();
    const auto path = DumpFilePath(dir);

    auto scope_time = tracing::Span::CurrentSpan().CreateScopeTime("dump");
    cache::dump::FileWriter writer(path, boost::filesystem::perms::owner_read,
                                   scope_time);
    WriteStringViewUnsafe(writer, {});
    writer.Finish();

    EXPECT_EQ(fs::blocking::ReadFileContents(path), "");

    cache::dump::FileReader reader(path);
    EXPECT_EQ(ReadStringViewUnsafe(reader, 0), "");
    reader.Finish();
  });
}

TEST(CacheDumpOperationsFile, Overread) {
  const auto file = fs::blocking::TempFile::Create();
  fs::blocking::RewriteFileContents(file.GetPath(), std::string(10, 'a'));

  cache::dump::FileReader reader(file.GetPath());
  try {
    ReadStringViewUnsafe(reader, 11);
  } catch (const cache::dump::Error& ex) {
    EXPECT_EQ(std::string{ex.what()},
              "Unexpected end-of-file while trying to read from the dump file: "
              "requested-size=11");
    return;
  }
  FAIL();
}

TEST(CacheDumpOperationsFile, Underread) {
  const auto file = fs::blocking::TempFile::Create();
  fs::blocking::RewriteFileContents(file.GetPath(), std::string(10, 'a'));

  cache::dump::FileReader reader(file.GetPath());
  EXPECT_EQ(ReadStringViewUnsafe(reader, 9), std::string(9, 'a'));
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
