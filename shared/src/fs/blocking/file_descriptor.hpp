#pragma once

#include <string>
#include <string_view>

#include <fs/blocking/open_mode.hpp>

namespace fs::blocking {

class FileDescriptor final {
 public:
  FileDescriptor();
  FileDescriptor(FileDescriptor&& other) noexcept;
  FileDescriptor& operator=(FileDescriptor&& other) noexcept;
  ~FileDescriptor();

  static FileDescriptor CreateTempFile();
  static FileDescriptor CreateTempFile(std::string pattern);
  static FileDescriptor OpenFile(std::string filename, OpenMode flags,
                                 int perms = 0600);
  static FileDescriptor OpenDirectory(std::string directory, OpenMode flags);

  void FSync();
  void Close();
  void Write(std::string_view contents);

  std::string ReadContents();

  int GetNativeFd() { return fd_; }
  int Release() &&;

  std::size_t GetSize() const;

  const std::string& GetPath() const { return path_; }

 private:
  FileDescriptor(int fd, std::string&& path);
  auto GetFileStats() const;

  int fd_;
  std::string path_;
};

}  // namespace fs::blocking
