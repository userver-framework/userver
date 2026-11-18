#include <userver/fs/blocking/temp_file.hpp>

#include <filesystem>
#include <stdexcept>
#include <utility>

#include <gtest/gtest.h>

#include <userver/fs/blocking/temp_directory.hpp>
#include <userver/fs/blocking/write.hpp>
#include <userver/logging/log.hpp>
#include <userver/utest/log_capture_fixture.hpp>

USERVER_NAMESPACE_BEGIN

namespace {

using FsBlockingTempFileWithLog = utest::LogCaptureFixture<>;

bool StartsWith(std::string_view hay, std::string_view needle) { return hay.substr(0, needle.size()) == needle; }

}  // namespace

TEST(TempFile, CreateDestroy) {
    std::string path;
    {
        const auto file = fs::blocking::TempFile::Create();
        path = file.GetPath();
        EXPECT_TRUE(std::filesystem::exists(path));
    }
    EXPECT_FALSE(std::filesystem::exists(path));
}

TEST(TempFile, DestroysInStackUnwinding) {
    std::string path;
    try {
        const auto file = fs::blocking::TempFile::Create();
        path = file.GetPath();
        throw std::runtime_error("test");
    } catch (const std::runtime_error& ex) {
        EXPECT_EQ(ex.what(), std::string{"test"});
        EXPECT_FALSE(std::filesystem::exists(path));
        return;
    }
    FAIL();
}

TEST(TempFile, Permissions) {
    const auto file = fs::blocking::TempFile::Create();

    const auto status = std::filesystem::status(file.GetPath());
    EXPECT_EQ(status.type(), std::filesystem::file_type::regular);
    EXPECT_EQ(status.permissions(), std::filesystem::perms::owner_read | std::filesystem::perms::owner_write);
}

TEST(TempFile, EarlyRemove) {
    std::string path;

    auto file = fs::blocking::TempFile::Create();
    path = file.GetPath();
    fs::blocking::RewriteFileContents(path, "foo");

    EXPECT_NO_THROW(std::move(file).Remove());
    EXPECT_FALSE(std::filesystem::exists(path));
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
    EXPECT_EQ(std::filesystem::status(root + "/foo").permissions(), std::filesystem::perms::owner_all);
}

TEST_F(FsBlockingTempFileWithLog, BlockingTempFileDestructorWithException) {
    std::string parent_dir;
    {
        const auto parent = fs::blocking::TempDirectory::Create();
        parent_dir = parent.GetPath();

        {
            // Create temp file in temporaty directory
            const auto file = fs::blocking::TempFile::Create(parent_dir, "BlockingTempFileDestructorWithException");

            const auto dir_status = std::filesystem::status(parent_dir);
            const auto original_perms = dir_status.permissions();

            LOG_DEBUG() << "Original directory permissions: " << static_cast<int>(original_perms);

            // Check that the directory has write permissions
            ASSERT_TRUE((original_perms & std::filesystem::perms::owner_write) != std::filesystem::perms::none);

            // Change permissions for the directory
            std::filesystem::permissions(
                parent_dir,
                std::filesystem::perms::owner_read | std::filesystem::perms::owner_exec |
                    std::filesystem::perms::group_read | std::filesystem::perms::group_exec |
                    std::filesystem::perms::others_read | std::filesystem::perms::others_exec
            );
            LOG_DEBUG() << "Changed directory permissions, write is now forbidden";

            // Check that the file can not be removed
            try {
                std::filesystem::remove(file.GetPath());
                FAIL() << "Expected exception when removing file";
            } catch (const std::exception& ex) {
                LOG_DEBUG() << "Confirmed file cannot be removed";
            }

            // We don't expect that ~TempFile to throw an exception
        }

        // Check that log with error from ~TempFile exists
        EXPECT_EQ(GetLogCapture().Filter("fs::blocking::~TempFile failed with exception:").size(), 1);

        // Rollback directory permissions
        std::filesystem::permissions(
            parent_dir,
            std::filesystem::perms::owner_read | std::filesystem::perms::owner_write |
                std::filesystem::perms::owner_exec | std::filesystem::perms::group_read |
                std::filesystem::perms::group_write | std::filesystem::perms::group_exec |
                std::filesystem::perms::others_read | std::filesystem::perms::others_write |
                std::filesystem::perms::others_exec
        );
        LOG_DEBUG() << "Changed directory permissions, write is now available";
    }
    ASSERT_FALSE(std::filesystem::exists(parent_dir));
}

USERVER_NAMESPACE_END
