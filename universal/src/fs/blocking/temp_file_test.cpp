#include <userver/fs/blocking/temp_file.hpp>

#include <utility>

#include <gtest/gtest.h>
#include <boost/filesystem/operations.hpp>

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
    EXPECT_EQ(status.permissions(), boost::filesystem::perms::owner_read | boost::filesystem::perms::owner_write);
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
    EXPECT_EQ(boost::filesystem::status(root + "/foo").permissions(), boost::filesystem::perms::owner_all);
}

TEST_F(FsBlockingTempFileWithLog, BlockingTempFileDestructorWithException) {
    std::string parent_dir;
    {
        const auto parent = fs::blocking::TempDirectory::Create();
        parent_dir = parent.GetPath();

        {
            // Create temp file in temporaty directory
            const auto file = fs::blocking::TempFile::Create(parent_dir, "BlockingTempFileDestructorWithException");

            const auto dir_status = boost::filesystem::status(parent_dir);
            const auto original_perms = dir_status.permissions();

            LOG_DEBUG() << "Original directory permissions: " << static_cast<int>(original_perms);

            // Check that the directory has write permissions
            ASSERT_TRUE((original_perms & boost::filesystem::perms::owner_write) != boost::filesystem::perms::no_perms);

            // Change permissions for the directory
            boost::filesystem::permissions(
                parent_dir,
                boost::filesystem::perms::owner_read | boost::filesystem::perms::owner_exe |
                    boost::filesystem::perms::group_read | boost::filesystem::perms::group_exe |
                    boost::filesystem::perms::others_read | boost::filesystem::perms::others_exe
            );
            LOG_DEBUG() << "Changed directory permissions, write is now forbidden";

            // Check that the file can not be removed
            try {
                boost::filesystem::remove(file.GetPath());
                FAIL() << "Expected exception when removing file";
            } catch (const std::exception& ex) {
                LOG_DEBUG() << "Confirmed file cannot be removed";
            }

            // We don't expect that ~TempFile to throw an exception
        }

        // Check that log with error from ~TempFile exists
        EXPECT_EQ(GetLogCapture().Filter("fs::blocking::~TempFile failed with exception:").size(), 1);

        // Rollback directory permissions
        boost::filesystem::permissions(
            parent_dir,
            boost::filesystem::perms::owner_read | boost::filesystem::perms::owner_write |
                boost::filesystem::perms::owner_exe | boost::filesystem::perms::group_read |
                boost::filesystem::perms::group_write | boost::filesystem::perms::group_exe |
                boost::filesystem::perms::others_read | boost::filesystem::perms::others_write |
                boost::filesystem::perms::others_exe
        );
        LOG_DEBUG() << "Changed directory permissions, write is now available";
    }
    ASSERT_FALSE(boost::filesystem::exists(parent_dir));
}

USERVER_NAMESPACE_END
