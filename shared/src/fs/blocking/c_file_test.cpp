#include <fs/blocking/c_file.hpp>

#include <gtest/gtest.h>

#include <fs/blocking/read.hpp>
#include <fs/blocking/temp_directory.hpp>
#include <fs/blocking/write.hpp>

TEST(CFile, NullFile) {
  fs::blocking::CFile file;
  EXPECT_FALSE(file.IsOpen());
}

TEST(CFile, Move) {
  fs::blocking::TempDirectory dir;
  const std::string path = dir.GetPath() + "/foo";
  fs::blocking::RewriteFileContents(path, "bar");

  fs::blocking::CFile file1(path, fs::blocking::OpenFlag::kRead);
  EXPECT_TRUE(file1.IsOpen());

  fs::blocking::CFile file2 = std::move(file1);
  EXPECT_FALSE(file1.IsOpen());
  EXPECT_TRUE(file2.IsOpen());

  fs::blocking::CFile file3;
  EXPECT_FALSE(file3.IsOpen());
  file3 = std::move(file2);
  EXPECT_FALSE(file2.IsOpen());
  EXPECT_TRUE(file3.IsOpen());

  const std::string path2 = dir.GetPath() + "/baz";
  fs::blocking::RewriteFileContents(path, "qux");
  fs::blocking::CFile file4(path, fs::blocking::OpenFlag::kRead);
  EXPECT_TRUE(file4.IsOpen());
  file4 = std::move(file3);
  EXPECT_FALSE(file3.IsOpen());
  EXPECT_TRUE(file4.IsOpen());
}

TEST(CFile, Reading) {
  fs::blocking::TempDirectory dir;
  const std::string path = dir.GetPath() + "/foo";
  fs::blocking::RewriteFileContents(path, "bar");

  fs::blocking::CFile file(path, fs::blocking::OpenFlag::kRead);
  EXPECT_TRUE(file.IsOpen());

  std::string buffer(10, '\0');
  EXPECT_EQ(file.Read(buffer.data(), 1), 1);
  EXPECT_EQ(file.Read(buffer.data(), 10), 2);

  std::move(file).Close();
  EXPECT_FALSE(file.IsOpen());
}

TEST(CFile, Writing) {
  fs::blocking::TempDirectory dir;
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
