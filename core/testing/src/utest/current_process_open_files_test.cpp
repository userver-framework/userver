#include <userver/utest/current_process_open_files.hpp>

#include <algorithm>

#include <fmt/format.h>

#include <userver/fs/blocking/file_descriptor.hpp>
#include <userver/fs/blocking/temp_file.hpp>

#include <gtest/gtest.h>

USERVER_NAMESPACE_BEGIN

TEST(CurrentProcessOpenFiles, DISABLED_Basic) {
  auto file = fs::blocking::TempFile::Create();
  auto fd = fs::blocking::FileDescriptor::Open(
      file.GetPath(), {fs::blocking::OpenFlag::kCreateIfNotExists,
                       fs::blocking::OpenFlag::kWrite});

  const auto opened_files = utest::CurrentProcessOpenFiles();
  const auto it =
      std::find(opened_files.begin(), opened_files.end(), file.GetPath());
  EXPECT_NE(it, opened_files.end()) << fmt::format(
      "Failed to find open file '{}'. Detected open files: ", file.GetPath(),
      fmt::join(opened_files, ", "));
}

USERVER_NAMESPACE_END
