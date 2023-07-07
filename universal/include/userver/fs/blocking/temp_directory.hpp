#pragma once

/// @file userver/fs/blocking/temp_directory.hpp
/// @brief @copybrief fs::blocking::TempDirectory

#include <string>
#include <string_view>

USERVER_NAMESPACE_BEGIN

namespace fs::blocking {

/// @ingroup userver_containers
///
/// @brief A unique directory for temporary files. The directory is deleted when
/// the `TempDirectory` is destroyed.
/// @note The directory, as well as any newly created parent directories,
/// has permissions=0700.
class TempDirectory final {
 public:
  /// @brief Create the directory at the default path for temporary files
  /// @throws std::runtime_error
  static TempDirectory Create();

  /// @brief Create the directory at the specified path
  /// @param parent_path The directory where the temporary directory
  /// will be created
  /// @param name_prefix Directory name prefix, a random string will be added
  /// after the prefix
  /// @throws std::runtime_error
  static TempDirectory Create(std::string_view parent_path,
                              std::string_view name_prefix);

  TempDirectory() = default;
  TempDirectory(TempDirectory&& other) noexcept;
  TempDirectory& operator=(TempDirectory&& other) noexcept;
  ~TempDirectory();

  /// Take ownership of an existing directory
  static TempDirectory Adopt(std::string path);

  /// The directory path
  const std::string& GetPath() const;

  /// @brief Remove the directory early
  /// @throws std::runtime_error
  void Remove() &&;

 private:
  explicit TempDirectory(std::string&& path);

  std::string path_;
};

}  // namespace fs::blocking

USERVER_NAMESPACE_END
