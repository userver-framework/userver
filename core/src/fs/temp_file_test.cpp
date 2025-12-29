#include <gmock/gmock.h>
#include <userver/utest/utest.hpp>

#include <boost/filesystem/operations.hpp>

#include <userver/engine/task/current_task.hpp>
#include <userver/fs/blocking/temp_directory.hpp>
#include <userver/fs/temp_file.hpp>
#include <userver/logging/log.hpp>
#include <userver/utest/log_capture_fixture.hpp>

USERVER_NAMESPACE_BEGIN

namespace {
using FsTempFileWithLog = utest::LogCaptureFixture<>;
}

UTEST_F(FsTempFileWithLog, TempFileDestructorWithException) {
    std::string parent_dir;
    {
        const auto parent = fs::blocking::TempDirectory::Create();
        parent_dir = parent.GetPath();

        {
            // Create temp file in temporaty directory
            const auto file = fs::TempFile::Create(
                parent_dir,
                "TempFileDestructorWithException",
                engine::current_task::GetTaskProcessor()
            );

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
        EXPECT_THAT(GetLogCapture().Filter("fs::~TempFile failed with exception:"), testing::SizeIs(1));

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
