#pragma once

/// @file userver/fs/blocking/file_descriptor.hpp
/// @brief @copybrief fs::blocking::FileDescriptor

#include <string>
#include <string_view>

#include <boost/filesystem/operations.hpp>

#include <userver/fs/blocking/open_mode.hpp>

USERVER_NAMESPACE_BEGIN

namespace fs::blocking {

/// @ingroup userver_containers
///
/// @brief A file descriptor wrapper
/// @details The file is closed in the destructor
/// @note The operations on the file are blocking and not thread-safe
class FileDescriptor final {
 public:
  /// @brief Open a file using ::open
  /// @throws std::runtime_error
  static FileDescriptor Open(
      const std::string& path, OpenMode flags,
      boost::filesystem::perms perms = boost::filesystem::perms::owner_read |
                                       boost::filesystem::perms::owner_write);

  /// @brief Open a directory node
  /// @note The only valid operation for such a `FileDescriptor` is `FSync`.
  /// @throws std::runtime_error
  static FileDescriptor OpenDirectory(const std::string& path);

  /// @brief Use the file discriptor directly
  static FileDescriptor AdoptFd(int fd) noexcept;

  FileDescriptor() = delete;
  FileDescriptor(FileDescriptor&& other) noexcept;
  FileDescriptor& operator=(FileDescriptor&& other) noexcept;
  ~FileDescriptor();

  /// @brief Checks if the file is open
  /// @note Operations can only be performed on an open `FileDescriptor`.
  bool IsOpen() const;

  /// @brief Closes the file manually
  /// @throws std::runtime_error
  void Close() &&;

  /// Returns the native file handle
  int GetNative() const;

  /// Passes the ownership of the file descriptor to the caller
  int Release() &&;

  /// @brief Writes data to the file
  /// @warning Unless `FSync` is called, there is no guarantee the data
  /// is stored on disk safely.
  /// @throws std::runtime_error
  void Write(std::string_view contents);

  /// @brief Reads data from the file at current offset
  /// @returns The amount of bytes actually acquired, which can be equal
  /// to `max_size`, or less on end-of-file
  /// @throws std::runtime_error
  std::size_t Read(char* buffer, std::size_t max_size);

  /// @brief Sets the file read/write offset from the beginning of the file
  /// @throws std::runtime_error
  void Seek(std::size_t offset_in_bytes);

  /// @brief Makes sure the written data is actually stored on disk
  /// @throws std::runtime_error
  void FSync();

  /// @brief Fetches the file size
  /// @throws std::runtime_error
  std::size_t GetSize() const;

 private:
  explicit FileDescriptor(int fd);

  friend class TempFile;

  int fd_;
};

}  // namespace fs::blocking

USERVER_NAMESPACE_END
