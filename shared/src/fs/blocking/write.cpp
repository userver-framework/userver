#include <fs/blocking/write.hpp>

#include <fcntl.h>

#include <memory>
#include <stdexcept>
#include <system_error>

#include <boost/core/ignore_unused.hpp>

#include <fs/blocking/file_descriptor.hpp>
#include <utils/strerror.hpp>

namespace fs::blocking {

void CreateDirectories(const std::string& path) {
  boost::system::error_code errc;
  boost::filesystem::create_directories(path, errc);
  if (!!errc)
    throw std::system_error(
        // static_cast is valid because both boost::error_code and std::errc map
        // to the same `errno` constants
        std::make_error_code(static_cast<std::errc>(errc.value())),
        "Failed to create directories under '" + path + "'");
}

void RewriteFileContents(const std::string& path, std::string_view contents) {
  auto fd = FileDescriptor::OpenFile(
      path, utils::Flags<FileDescriptor::OpenMode>() |
                FileDescriptor::OpenMode::kWrite |
                FileDescriptor::OpenMode::kCreateIfNotExists);

  fd.Write(contents);
  fd.FSync();
  fd.Close();
}

void SyncDirectoryContents(const std::string& path) {
  auto fd =
      FileDescriptor::OpenDirectory(path, FileDescriptor::OpenMode::kRead);
  fd.FSync();
  fd.Close();
}

void Rename(const std::string& source, const std::string& destination) {
  boost::filesystem::rename(source, destination);
}

void Chmod(const std::string& path, boost::filesystem::perms perms) {
  boost::filesystem::permissions(path, perms);
}

bool RemoveSingleFile(const std::string& path) {
  return boost::filesystem::remove(path);
}

}  // namespace fs::blocking
