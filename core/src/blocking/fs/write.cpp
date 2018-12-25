#include <blocking/fs/write.hpp>

#include <fcntl.h>

#include <memory>
#include <stdexcept>
#include <system_error>

#include <boost/core/ignore_unused.hpp>

#include <blocking/fs/file_descriptor.hpp>
#include <utils/strerror.hpp>

namespace blocking {
namespace fs {

void RewriteFileContents(const std::string& path, std::string contents) {
  auto fd = FileDescriptor::OpenFile(
      path.c_str(), utils::Flags<FileDescriptor::OpenMode>() |
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

}  // namespace fs
}  // namespace blocking
