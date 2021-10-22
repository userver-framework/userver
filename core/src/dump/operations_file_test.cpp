#include <userver/dump/operations_file.hpp>

#include <boost/regex.hpp>

#include <userver/dump/unsafe.hpp>
#include <userver/fs/blocking/read.hpp>
#include <userver/fs/blocking/temp_directory.hpp>
#include <userver/fs/blocking/temp_file.hpp>
#include <userver/fs/blocking/write.hpp>
#include <userver/tracing/span.hpp>
#include <userver/utest/utest.hpp>

USERVER_NAMESPACE_BEGIN

namespace {

std::string DumpFilePath(const fs::blocking::TempDirectory& dir) {
  return dir.GetPath() + "/dump";
}

}  // namespace

UTEST(DumpOperationsFile, WriteReadRaw) {
  const auto dir = fs::blocking::TempDirectory::Create();
  const auto path = DumpFilePath(dir);

  constexpr std::size_t kMaxLength = 10;
  std::size_t total_length = 0;

  auto scope_time = tracing::Span::CurrentSpan().CreateScopeTime("dump");
  dump::FileWriter writer(path, boost::filesystem::perms::owner_read,
                          scope_time);
  for (std::size_t i = 0; i <= kMaxLength; ++i) {
    WriteStringViewUnsafe(writer, std::string(i, 'a'));
    total_length += i;
  }
  writer.Finish();

  EXPECT_EQ(fs::blocking::ReadFileContents(path),
            std::string(total_length, 'a'));

  dump::FileReader reader(path);
  for (std::size_t i = 0; i <= kMaxLength; ++i) {
    EXPECT_EQ(ReadStringViewUnsafe(reader, i), std::string(i, 'a'));
  }
  reader.Finish();
}

UTEST(DumpOperationsFile, EmptyDump) {
  const auto dir = fs::blocking::TempDirectory::Create();
  const auto path = DumpFilePath(dir);

  auto scope_time = tracing::Span::CurrentSpan().CreateScopeTime("dump");
  dump::FileWriter writer(path, boost::filesystem::perms::owner_read,
                          scope_time);
  writer.Finish();

  EXPECT_EQ(fs::blocking::ReadFileContents(path), "");

  dump::FileReader reader(path);
  reader.Finish();
}

UTEST(DumpOperationsFile, EmptyStringDump) {
  const auto dir = fs::blocking::TempDirectory::Create();
  const auto path = DumpFilePath(dir);

  auto scope_time = tracing::Span::CurrentSpan().CreateScopeTime("dump");
  dump::FileWriter writer(path, boost::filesystem::perms::owner_read,
                          scope_time);
  WriteStringViewUnsafe(writer, {});
  writer.Finish();

  EXPECT_EQ(fs::blocking::ReadFileContents(path), "");

  dump::FileReader reader(path);
  EXPECT_EQ(ReadStringViewUnsafe(reader, 0), "");
  reader.Finish();
}

TEST(DumpOperationsFile, Overread) {
  const auto file = fs::blocking::TempFile::Create();
  fs::blocking::RewriteFileContents(file.GetPath(), std::string(10, 'a'));

  dump::FileReader reader(file.GetPath());
  try {
    ReadStringViewUnsafe(reader, 11);
  } catch (const dump::Error& ex) {
    EXPECT_EQ(std::string{ex.what()},
              "Unexpected end-of-file while trying to read from the dump file: "
              "requested-size=11");
    return;
  }
  FAIL();
}

TEST(DumpOperationsFile, Underread) {
  const auto file = fs::blocking::TempFile::Create();
  fs::blocking::RewriteFileContents(file.GetPath(), std::string(10, 'a'));

  dump::FileReader reader(file.GetPath());
  EXPECT_EQ(ReadStringViewUnsafe(reader, 9), std::string(9, 'a'));
  try {
    reader.Finish();
  } catch (const dump::Error& ex) {
    EXPECT_TRUE(boost::regex_match(
        ex.what(),
        boost::regex{"Unexpected extra data at the end of the dump file "
                     "\".+\": file-size=10, position=9, unread-size=1"}))
        << ex.what();
    return;
  }
  FAIL();
}

USERVER_NAMESPACE_END
