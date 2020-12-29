#include <fs/blocking/write.hpp>

#include <gtest/gtest.h>
#include <boost/filesystem.hpp>

#include <fs/blocking/temp_directory.hpp>

TEST(Fs, CreateDirectories) {
  fs::blocking::TempDirectory root;
  const auto perms = boost::filesystem::perms::owner_all;
  fs::blocking::CreateDirectories(root.GetPath() + "//foo/bar///baz/", perms);

  auto status = boost::filesystem::status(root.GetPath() + "/foo/bar/baz");
  EXPECT_TRUE(status.type() == boost::filesystem::directory_file);
  EXPECT_TRUE(status.permissions() == perms);
}
