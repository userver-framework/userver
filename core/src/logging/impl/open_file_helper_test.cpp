#include "open_file_helper.hpp"

#include <stdexcept>

#include <gmock/gmock.h>
#include <boost/filesystem.hpp>

#include <userver/fs/blocking/temp_directory.hpp>

USERVER_NAMESPACE_BEGIN

TEST(OpenFileHelperTest, ReopenFail) {
  const fs::blocking::TempDirectory temp_root =
      fs::blocking::TempDirectory::Create();
  const std::string filename = temp_root.GetPath() + "/temp_file";
  EXPECT_NO_THROW(
      logging::impl::OpenFile<fs::blocking::FileDescriptor>(filename));
  boost::filesystem::permissions(filename, boost::filesystem::perms::no_perms);
  EXPECT_THROW(logging::impl::OpenFile<fs::blocking::FileDescriptor>(filename),
               std::runtime_error);
  try {
    logging::impl::OpenFile<fs::blocking::FileDescriptor>(filename);
  } catch (std::runtime_error& e) {
    const std::string err = e.what();
    EXPECT_TRUE(err.rfind("Filename ", 0) == 0);
    const std::string end = " cannot be created or opened";
    EXPECT_TRUE(end.size() < err.size());
    EXPECT_TRUE(std::equal(end.rbegin(), end.rend(), err.rbegin()));
  }
}

USERVER_NAMESPACE_END
