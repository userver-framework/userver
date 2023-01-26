#pragma once

/// @file userver/fs/temp_file.hpp
/// @brief @copybrief fs::TempFile

#include <string>
#include <string_view>

#include <userver/engine/task/task_processor_fwd.hpp>
#include <userver/fs/blocking/temp_file.hpp>

USERVER_NAMESPACE_BEGIN

namespace fs {

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
  static TempFile Create(engine::TaskProcessor& fs_task_processor);

  /// @brief Create the file at the specified path
  /// @param parent_path The directory where the temporary file will be created
  /// @param name_prefix File name prefix, a random string will be added
  /// after the prefix
  /// @throws std::runtime_error
  static TempFile Create(std::string_view parent_path,
                         std::string_view name_prefix,
                         engine::TaskProcessor& fs_task_processor);

  TempFile() = delete;
  TempFile(TempFile&& other) noexcept;
  TempFile& operator=(TempFile&& other) noexcept;
  ~TempFile();

  /// Take ownership of an existing file
  static TempFile Adopt(std::string path,
                        engine::TaskProcessor& fs_task_processor);

  /// The file path
  const std::string& GetPath() const;

  /// @brief Remove the file early
  /// @throws std::runtime_error
  void Remove() &&;

 private:
  TempFile(engine::TaskProcessor& fs_task_processor,
           fs::blocking::TempFile temp_file);

  engine::TaskProcessor& fs_task_processor_;
  fs::blocking::TempFile temp_file_;
};

}  // namespace fs

USERVER_NAMESPACE_END
