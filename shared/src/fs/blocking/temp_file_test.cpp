#include <userver/fs/blocking/temp_file.hpp>

#include <utility>

#include <gtest/gtest.h>
#include <boost/filesystem/operations.hpp>

#include <userver/fs/blocking/temp_directory.hpp>
#include <userver/fs/blocking/write.hpp>

USERVER_NAMESPACE_BEGIN

namespace {

bool StartsWith(std::string_view hay, std::string_view needle) {
  return hay.substr(0, needle.size()) == needle;
}

}  // namespace

TEST(TempFile, CreateDestroy) {
  std::string path;
  {
    const auto file = fs::blocking::TempFile::Create();
    path = file.GetPath();
    EXPECT_TRUE(boost::filesystem::exists(path));
  }
  EXPECT_FALSE(boost::filesystem::exists(path));
}

TEST(TempFile, DestroysInStackUnwinding) {
  std::string path;
  try {
    const auto file = fs::blocking::TempFile::Create();
    path = file.GetPath();
    throw std::runtime_error("test");
  } catch (const std::runtime_error& ex) {
    EXPECT_EQ(ex.what(), std::string{"test"});
    EXPECT_FALSE(boost::filesystem::exists(path));
    return;
  }
  FAIL();
}

TEST(TempFile, Permissions) {
  const auto file = fs::blocking::TempFile::Create();

  const auto status = boost::filesystem::status(file.GetPath());
  EXPECT_EQ(status.type(), boost::filesystem::regular_file);
  EXPECT_EQ(status.permissions(), boost::filesystem::perms::owner_read |
                                      boost::filesystem::perms::owner_write);
}

TEST(TempFile, EarlyRemove) {
  std::string path;

  auto file = fs::blocking::TempFile::Create();
  path = file.GetPath();
  fs::blocking::RewriteFileContents(path, "foo");

  EXPECT_NO_THROW(std::move(file).Remove());
  EXPECT_FALSE(boost::filesystem::exists(path));
}

TEST(TempFile, DoubleRemove) {
  auto file = fs::blocking::TempFile::Create();
  EXPECT_NO_THROW(std::move(file).Remove());
  // NOLINTNEXTLINE(bugprone-use-after-move)
  EXPECT_THROW(std::move(file).Remove(), std::runtime_error);
}

TEST(TempFile, CustomPath) {
  const auto parent = fs::blocking::TempDirectory::Create();
  const auto& root = parent.GetPath();

  const auto child = fs::blocking::TempFile::Create(root + "/foo", "bar");
  EXPECT_TRUE(StartsWith(child.GetPath(), root + "/foo/bar"));
  EXPECT_EQ(boost::filesystem::status(root + "/foo").permissions(),
            boost::filesystem::perms::owner_all);
}

USERVER_NAMESPACE_END
