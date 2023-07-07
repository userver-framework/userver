#include <userver/fs/blocking/write.hpp>

#include <gtest/gtest.h>
#include <boost/filesystem.hpp>

#include <userver/fs/blocking/temp_directory.hpp>

USERVER_NAMESPACE_BEGIN

TEST(Fs, CreateDirectories) {
  const auto root = fs::blocking::TempDirectory::Create();
  const auto perms = boost::filesystem::perms::owner_all;
  fs::blocking::CreateDirectories(root.GetPath() + "//foo/bar///baz/", perms);

  auto status = boost::filesystem::status(root.GetPath() + "/foo/bar/baz");
  EXPECT_TRUE(status.type() == boost::filesystem::directory_file);
  EXPECT_TRUE(status.permissions() == perms);
}

USERVER_NAMESPACE_END
