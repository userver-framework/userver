#pragma once

#include <fcntl.h>

#include <string>

#include <utils/flags.hpp>

namespace fs {
namespace blocking {

class FileDescriptor {
 public:
  enum class OpenMode {
    kNone,  // For utils::Flags<>
    kRead = O_RDONLY,
    kWrite = O_WRONLY,
    kCreateIfNotExists = O_CREAT,
    kExclusiveCreate = O_EXCL
  };
  using OpenFlags = utils::Flags<OpenMode>;

  FileDescriptor() = default;
  FileDescriptor(FileDescriptor&& other) noexcept;
  FileDescriptor& operator=(FileDescriptor&& other) noexcept;
  ~FileDescriptor();

  static FileDescriptor CreateTempFile(std::string pattern);
  static FileDescriptor OpenFile(std::string filename, OpenFlags flags,
                                 int mode = 0600);
  static FileDescriptor OpenDirectory(std::string directory, OpenFlags flags);
  static FileDescriptor FromFd(int fd, std::string filename);

  void FSync();
  void Close();
  void Write(const std::string& contents);

  std::string ReadContents();

  int GetNativeFd() { return fd_; }
  int Release() &&;

  std::size_t GetSize() const;

  const std::string& GetPath() const { return path_; }

 private:
  FileDescriptor(int fd, std::string path);
  auto GetFileStats() const;

  static FileDescriptor FromFdChecked(int fd, std::string filename);

  int fd_ = -1;
  std::string path_;
};

}  // namespace blocking
}  // namespace fs
