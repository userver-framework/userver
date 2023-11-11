#include "open_file_helper.hpp"

#include <stdexcept>

#include <fmt/format.h>
#include <gmock/gmock.h>
#include <boost/filesystem.hpp>

#include <userver/fs/blocking/temp_directory.hpp>
#include <userver/utest/assert_macros.hpp>

USERVER_NAMESPACE_BEGIN

TEST(OpenFileHelperTest, ReopenFail) {
  const fs::blocking::TempDirectory temp_root =
      fs::blocking::TempDirectory::Create();
  const std::string filename = temp_root.GetPath() + "/temp_file";
  EXPECT_NO_THROW(
      logging::impl::OpenFile<fs::blocking::FileDescriptor>(filename));
  boost::filesystem::permissions(filename, boost::filesystem::perms::no_perms);
  UASSERT_THROW_MSG(
      logging::impl::OpenFile<fs::blocking::FileDescriptor>(filename),
      std::runtime_error,
      fmt::format("Filename {} cannot be created or opened", filename));
}

USERVER_NAMESPACE_END
