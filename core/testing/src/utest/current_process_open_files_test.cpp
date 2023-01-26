#include <userver/utest/current_process_open_files.hpp>

#include <sys/param.h>

#include <algorithm>

#include <fmt/format.h>
#include <boost/filesystem.hpp>

#include <userver/fs/blocking/file_descriptor.hpp>
#include <userver/fs/blocking/temp_file.hpp>

#include <gtest/gtest.h>

USERVER_NAMESPACE_BEGIN

constexpr std::string_view kTestFilePart = "test_files_listing_of_current_proc";

// Mostly tests that Subprocess.CheckSpdlogClosesFds test would detect
// non-closed file descriptors.
#if defined(BSD)
// /dev/fd/* are not symlinks
TEST(DISABLED_CurrentProcessOpenFiles, Basic) {
#else
TEST(CurrentProcessOpenFiles, Basic) {
#endif
  const auto file_guard = fs::blocking::TempFile::Create("/tmp", kTestFilePart);
  const auto& path = file_guard.GetPath();

  const auto fd = fs::blocking::FileDescriptor::Open(
      path, {fs::blocking::OpenFlag::kCreateIfNotExists,
             fs::blocking::OpenFlag::kWrite});

  const auto opened_files = utest::CurrentProcessOpenFiles();
  // NOLINTNEXTLINE(readability-qualified-auto)
  const auto it = std::find_if(
      opened_files.begin(), opened_files.end(), [](const auto& file) {
        return file.find(kTestFilePart) != std::string::npos;
      });
  EXPECT_NE(it, opened_files.end())
      << fmt::format("Failed to find open file '{}'. Detected open files: {}",
                     path, fmt::join(opened_files, ", "));
}

USERVER_NAMESPACE_END
