#include <userver/fs/blocking/write.hpp>

#include <filesystem>

#include <gtest/gtest.h>

#include <userver/fs/blocking/temp_directory.hpp>

USERVER_NAMESPACE_BEGIN

TEST(Fs, CreateDirectories) {
    const auto root = fs::blocking::TempDirectory::Create();
    const auto perms = std::filesystem::perms::owner_all;
    fs::blocking::CreateDirectories(root.GetPath() + "//foo/bar///baz/", perms);

    auto status = std::filesystem::status(root.GetPath() + "/foo/bar/baz");
    EXPECT_TRUE(status.type() == std::filesystem::file_type::directory);
    EXPECT_TRUE(status.permissions() == perms);
}

USERVER_NAMESPACE_END
