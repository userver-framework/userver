#include <gtest/gtest.h>

#include <boost/filesystem/operations.hpp>

#include <fs/blocking/file_descriptor.hpp>

using FileDescriptor = fs::blocking::FileDescriptor;

TEST(FileDescriptor, CreateTempFile) {
  auto fd = FileDescriptor::CreateTempFile();
  EXPECT_TRUE(boost::filesystem::exists(fd.GetPath()));
}

TEST(FileDescriptor, OpenExisting) {
  auto fd = FileDescriptor::CreateTempFile();
  EXPECT_NE(-1, fd.GetNativeFd());

  EXPECT_EQ(0, fd.GetSize());

  EXPECT_NO_THROW(
      FileDescriptor::OpenFile(fd.GetPath(), fs::blocking::OpenFlag::kRead));

  EXPECT_THROW(FileDescriptor::OpenDirectory(fd.GetPath(),
                                             fs::blocking::OpenFlag::kRead),
               std::system_error);
  EXPECT_NO_THROW(fd.Close());
}

TEST(FileDescriptor, FSyncClose) {
  auto fd = FileDescriptor::CreateTempFile();
  EXPECT_NO_THROW(fd.Close());
  EXPECT_THROW(fd.FSync(), std::system_error);
  EXPECT_THROW(fd.Close(), std::system_error);
}

TEST(FileDescriptor, WriteRead) {
  auto fd = FileDescriptor::CreateTempFile();

  auto path = fd.GetPath();
  const auto str = "123456";

  EXPECT_NO_THROW(fd.Write(str));
  fd.Close();

  auto fd2 = FileDescriptor::OpenFile(path, fs::blocking::OpenFlag::kRead);
  EXPECT_EQ(str, fd2.ReadContents());
}
