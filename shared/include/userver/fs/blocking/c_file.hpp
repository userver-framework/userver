#pragma once

/// @file userver/fs/blocking/c_file.hpp
/// @brief @copybrief fs::blocking::CFile

#include <cstdio>
#include <string>
#include <string_view>

#include <boost/filesystem/operations.hpp>

#include <userver/fs/blocking/open_mode.hpp>
#include <userver/utils/fast_pimpl.hpp>

USERVER_NAMESPACE_BEGIN

namespace fs::blocking {

/// @ingroup userver_containers
///
/// @brief A `std::FILE*` wrapper
/// @details The file is closed in the destructor
/// @note The operations on the file are blocking and not thread-safe
class CFile final {
 public:
  /// Creates an empty file handle
  CFile();

  CFile(CFile&&) noexcept;
  CFile& operator=(CFile&&) noexcept;
  ~CFile();

  /// @brief Opens the file
  /// @throws std::runtime_error
  CFile(const std::string& path, OpenMode flags,
        boost::filesystem::perms perms = boost::filesystem::perms::owner_read |
                                         boost::filesystem::perms::owner_write);

  /// @brief Adopt the `std::FILE*` directly
  explicit CFile(std::FILE* file) noexcept;

  /// Checks if the file is open
  bool IsOpen() const;

  /// Returns the underlying file handle
  std::FILE* GetNative() &;

  /// Passes the ownership of the file to the caller
  std::FILE* Release() &&;

  /// @brief Closes the file manually
  /// @throws std::runtime_error
  void Close() &&;

  /// @brief Reads data from the file
  /// @returns The amount of bytes actually acquired, which can be equal
  /// to `max_size`, or less on end-of-file
  /// @throws std::runtime_error
  std::size_t Read(char* buffer, std::size_t max_size);

  /// @brief Writes data to the file
  /// @warning Unless `Flush` is called, there is no guarantee the file on disk
  /// is actually updated
  /// @throws std::runtime_error
  void Write(std::string_view data);

  /// @brief Synchronizes the written data with the file on disk
  /// @throws std::runtime_error
  void Flush();

  /// @brief Synchronizes the written data with the file on disk
  /// without fsync
  /// @throws std::runtime_error
  void FlushLight();

  /// @brief Fetches the current position in the file
  /// @throws std::runtime_error
  std::uint64_t GetPosition() const;

  /// @brief Fetches the file size
  /// @throws std::runtime_error
  std::uint64_t GetSize() const;

 private:
  struct Impl;
  utils::FastPimpl<Impl, sizeof(char*), alignof(char*)> impl_;
};

}  // namespace fs::blocking

USERVER_NAMESPACE_END
