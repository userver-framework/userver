#pragma once

/// @file userver/fs/blocking/temp_file.hpp
/// @brief @copybrief fs::blocking::TempFile

#include <string>
#include <string_view>

USERVER_NAMESPACE_BEGIN

namespace fs::blocking {

/// @ingroup userver_containers
///
/// @brief A unique temporary file. The file is deleted when the `TempFile`
/// object is destroyed.
/// @note The file has permissions=0600. Any newly created parent directories
/// have permissions=0700.
/// @note The newly created file is immediately opened with read-write
/// permissions. The file descriptor can be accessed using `File`.
class TempFile final {
 public:
  /// @brief Create the file at the default path for temporary files
  /// @throws std::runtime_error
  static TempFile Create();

  /// @brief Create the file at the specified path
  /// @param parent_path The directory where the temporary file will be created
  /// @param name_prefix File name prefix, a random string will be added
  /// after the prefix
  /// @throws std::runtime_error
  static TempFile Create(std::string_view parent_path,
                         std::string_view name_prefix);

  TempFile() = delete;
  TempFile(TempFile&& other) noexcept;
  TempFile& operator=(TempFile&& other) noexcept;
  ~TempFile();

  /// Take ownership of an existing file
  static TempFile Adopt(std::string path);

  /// The file path
  const std::string& GetPath() const;

  /// @brief Remove the file early
  /// @throws std::runtime_error
  void Remove() &&;

 private:
  explicit TempFile(std::string&& path);

  std::string path_;
};

}  // namespace fs::blocking

USERVER_NAMESPACE_END
