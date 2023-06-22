#include <userver/fs/blocking/c_file.hpp>

#include <gtest/gtest.h>

#include <userver/fs/blocking/read.hpp>
#include <userver/fs/blocking/temp_directory.hpp>
#include <userver/fs/blocking/temp_file.hpp>
#include <userver/fs/blocking/write.hpp>

USERVER_NAMESPACE_BEGIN

TEST(CFile, NullFile) {
  fs::blocking::CFile file;
  EXPECT_FALSE(file.IsOpen());
}

TEST(CFile, DirectPointer) {
  const auto file_scope = fs::blocking::TempFile::Create();
  fs::blocking::CFile file(std::fopen("file", "w"));
  EXPECT_TRUE(file.IsOpen());
  EXPECT_NO_THROW(file.Write("Test write data\n"));
}

TEST(CFile, Move) {
  const auto file1 = fs::blocking::TempFile::Create();
  fs::blocking::RewriteFileContents(file1.GetPath(), "bar");

  fs::blocking::CFile reader1(file1.GetPath(), fs::blocking::OpenFlag::kRead);
  EXPECT_TRUE(reader1.IsOpen());

  fs::blocking::CFile reader2 = std::move(reader1);
  EXPECT_TRUE(reader2.IsOpen());

  fs::blocking::CFile reader3;
  EXPECT_FALSE(reader3.IsOpen());
  reader3 = std::move(reader2);
  EXPECT_TRUE(reader3.IsOpen());

  const auto file2 = fs::blocking::TempFile::Create();
  fs::blocking::RewriteFileContents(file2.GetPath(), "qux");
  fs::blocking::CFile reader4(file2.GetPath(), fs::blocking::OpenFlag::kRead);
  EXPECT_TRUE(reader4.IsOpen());
  reader4 = std::move(reader3);
  EXPECT_TRUE(reader4.IsOpen());
}

TEST(CFile, Reading) {
  const auto file = fs::blocking::TempFile::Create();
  fs::blocking::RewriteFileContents(file.GetPath(), "bar");

  fs::blocking::CFile reader(file.GetPath(), fs::blocking::OpenFlag::kRead);
  EXPECT_TRUE(reader.IsOpen());

  std::string buffer(10, '\0');
  EXPECT_EQ(reader.Read(buffer.data(), 1), 1);
  EXPECT_EQ(reader.Read(buffer.data(), 10), 2);

  std::move(reader).Close();
  // NOLINTNEXTLINE(bugprone-use-after-move)
  EXPECT_FALSE(reader.IsOpen());
}

TEST(CFile, Writing) {
  const auto dir = fs::blocking::TempDirectory::Create();
  const std::string path = dir.GetPath() + "/foo";

  // file does not exist yet
  EXPECT_THROW(fs::blocking::CFile(path, fs::blocking::OpenFlag::kWrite),
               std::runtime_error);

  fs::blocking::CFile file(path, {fs::blocking::OpenFlag::kWrite,
                                  fs::blocking::OpenFlag::kCreateIfNotExists});
  file.Write("bar");
  file.Write("baz");
  file.Flush();

  EXPECT_EQ(fs::blocking::ReadFileContents(path), "barbaz");
}

TEST(CFile, WriteEmpty) {
  const auto dir = fs::blocking::TempDirectory::Create();
  const std::string path = dir.GetPath() + "/foo";

  fs::blocking::CFile file(path, {fs::blocking::OpenFlag::kWrite,
                                  fs::blocking::OpenFlag::kCreateIfNotExists});
  file.Write(std::string_view{});
  file.Flush();

  EXPECT_EQ(fs::blocking::ReadFileContents(path), "");
}

TEST(CFile, Position) {
  const auto dir = fs::blocking::TempDirectory::Create();
  const std::string path = dir.GetPath() + "/foo";

  {
    fs::blocking::CFile writer(path,
                               {fs::blocking::OpenFlag::kWrite,
                                fs::blocking::OpenFlag::kCreateIfNotExists});
    EXPECT_EQ(writer.GetPosition(), 0);
    EXPECT_EQ(writer.GetSize(), 0);

    writer.Write("CFile");
    EXPECT_EQ(writer.GetPosition(), 5);

    writer.Flush();
    EXPECT_EQ(writer.GetSize(), 5);
  }

  {
    std::string buffer(5, '\0');

    fs::blocking::CFile reader(path, fs::blocking::OpenFlag::kRead);
    EXPECT_EQ(reader.GetPosition(), 0);
    EXPECT_EQ(reader.GetSize(), 5);

    EXPECT_EQ(reader.Read(buffer.data(), 3), 3);
    EXPECT_EQ(reader.GetPosition(), 3);
    EXPECT_EQ(reader.GetSize(), 5);

    EXPECT_EQ(reader.Read(buffer.data(), 10), 2);
    EXPECT_EQ(reader.GetPosition(), 5);
    EXPECT_EQ(reader.GetSize(), 5);
  }
}

USERVER_NAMESPACE_END
