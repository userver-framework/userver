#include <gmock/gmock.h>
#include <userver/utest/utest.hpp>

#include <exception>
#include <filesystem>
#include <string>

#include <userver/engine/task/current_task.hpp>
#include <userver/fs/blocking/temp_directory.hpp>
#include <userver/fs/temp_file.hpp>
#include <userver/logging/log.hpp>
#include <userver/utest/log_capture_fixture.hpp>

USERVER_NAMESPACE_BEGIN

namespace {

using FsTempFileWithLog = utest::LogCaptureFixture<>;

}  // namespace

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
        EXPECT_THAT(GetLogCapture().Filter("fs::~TempFile failed with exception:"), testing::SizeIs(1));

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
