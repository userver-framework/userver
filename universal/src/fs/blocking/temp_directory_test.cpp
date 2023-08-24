#include <userver/fs/blocking/temp_directory.hpp>

#include <utility>

#include <gtest/gtest.h>
#include <boost/filesystem/operations.hpp>

#include <userver/fs/blocking/write.hpp>

USERVER_NAMESPACE_BEGIN

namespace {

bool StartsWith(std::string_view hay, std::string_view needle) {
  return hay.substr(0, needle.size()) == needle;
}

}  // namespace

TEST(TempDirectory, CreateDestroy) {
  std::string path;
  {
    const auto dir = fs::blocking::TempDirectory::Create();
    path = dir.GetPath();
    EXPECT_TRUE(boost::filesystem::exists(path));
  }
  EXPECT_FALSE(boost::filesystem::exists(path));
}

TEST(TempDirectory, DestroysInStackUnwinding) {
  std::string path;
  try {
    const auto dir = fs::blocking::TempDirectory::Create();
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
  const auto dir = fs::blocking::TempDirectory::Create();

  const auto status = boost::filesystem::status(dir.GetPath());
  EXPECT_EQ(status.type(), boost::filesystem::directory_file);
  EXPECT_EQ(status.permissions(), boost::filesystem::perms::owner_all);
}

TEST(TempDirectory, EarlyRemove) {
  std::string path;

  auto dir = fs::blocking::TempDirectory::Create();
  path = dir.GetPath();
  fs::blocking::RewriteFileContents(path + "/file", "blah");

  EXPECT_NO_THROW(std::move(dir).Remove());
  EXPECT_FALSE(boost::filesystem::exists(path));
}

TEST(TempDirectory, DoubleRemove) {
  auto dir = fs::blocking::TempDirectory::Create();
  EXPECT_NO_THROW(std::move(dir).Remove());
  // NOLINTNEXTLINE(bugprone-use-after-move)
  EXPECT_THROW(std::move(dir).Remove(), std::runtime_error);
}

TEST(TempDirectory, CustomPath) {
  const auto parent = fs::blocking::TempDirectory::Create();
  const auto& root = parent.GetPath();

  const auto child = fs::blocking::TempDirectory::Create(root + "/foo", "bar");
  EXPECT_TRUE(StartsWith(child.GetPath(), root + "/foo/bar"));
  EXPECT_EQ(boost::filesystem::status(root + "/foo").permissions(),
            boost::filesystem::perms::owner_all);
}

USERVER_NAMESPACE_END
