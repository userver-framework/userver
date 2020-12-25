#pragma once

#include <string>
#include <string_view>

#include <fs/blocking/open_mode.hpp>
#include <utils/fast_pimpl.hpp>

namespace fs::blocking {

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
  CFile(const std::string& path, OpenMode flags, int perms = 0600);

  /// Checks if the file is open
  bool IsOpen() const;

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

 private:
  struct Impl;
  utils::FastPimpl<Impl, sizeof(char*), sizeof(char*)> impl_;
};

}  // namespace fs::blocking
