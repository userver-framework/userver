#include <fs/blocking/temp_directory.hpp>

#include <utility>

#include <boost/algorithm/string/predicate.hpp>

#include <gtest/gtest.h>
#include <boost/filesystem/operations.hpp>

#include <fs/blocking/write.hpp>

TEST(TempDirectory, CreateDestroy) {
  std::string path;
  {
    fs::blocking::TempDirectory dir;
    path = dir.GetPath();
    EXPECT_TRUE(boost::filesystem::exists(path));
  }
  EXPECT_FALSE(boost::filesystem::exists(path));
}

TEST(TempDirectory, DestroysInStackUnwinding) {
  std::string path;
  try {
    fs::blocking::TempDirectory dir;
    path = dir.GetPath();
    throw std::runtime_error("test");
  } catch (const std::runtime_error& ex) {
    EXPECT_EQ(ex.what(), std::string{"test"});
    EXPECT_FALSE(boost::filesystem::exists(path));
    return;
  }
  FAIL();
}

TEST(TempDirectory, Permissions) {
  fs::blocking::TempDirectory dir;

  const auto status = boost::filesystem::status(dir.GetPath());
  EXPECT_EQ(status.type(), boost::filesystem::directory_file);
  EXPECT_EQ(status.permissions(), boost::filesystem::perms::owner_all);
}

TEST(TempDirectory, EarlyRemove) {
  std::string path;

  fs::blocking::TempDirectory dir;
  path = dir.GetPath();
  fs::blocking::RewriteFileContents(path + "/file", "blah");

  EXPECT_NO_THROW(std::move(dir).Remove());
  EXPECT_FALSE(boost::filesystem::exists(path));
}

TEST(TempDirectory, DoubleRemove) {
  fs::blocking::TempDirectory dir;
  EXPECT_NO_THROW(std::move(dir).Remove());
  EXPECT_THROW(std::move(dir).Remove(), std::runtime_error);
}

TEST(TempDirectory, CustomPath) {
  fs::blocking::TempDirectory parent;
  const auto root = parent.GetPath();

  fs::blocking::TempDirectory child(root + "/foo", "bar");
  EXPECT_TRUE(boost::starts_with(child.GetPath(), root + "/foo/bar"));
  EXPECT_EQ(boost::filesystem::status(root + "/foo").permissions(),
            boost::filesystem::perms::owner_all);
}
