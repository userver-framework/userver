#pragma once

#include <string>

namespace fs::blocking {

/// @brief A unique directory for temporary files. The directory is deleted when
/// the `TempDirectory` is destroyed.
/// @note The directory, as well as any newly created parent directories,
/// has permissions=0700.
class TempDirectory final {
 public:
  /// @brief Create the directory at the default path for temporary files
  /// @throws std::runtime_error
  TempDirectory();

  /// @brief Create the directory at the specified path
  /// @param parent_path The directory where the temporary directory
  /// will be created
  /// @param name_prefix Directory name prefix, a random string will be added
  /// after the prefix
  /// @throws std::runtime_error
  TempDirectory(std::string_view parent_path, std::string_view name_prefix);

  /// The directory path
  const std::string& GetPath() const;

  /// @brief Remove the directory early
  /// @throws std::runtime_error
  void Remove() &&;

  ~TempDirectory();
  TempDirectory(TempDirectory&& other) noexcept;
  TempDirectory& operator=(TempDirectory&& other) noexcept;

 private:
  std::string path_;
};

}  // namespace fs::blocking
