#pragma once

#include <string>

namespace fs::blocking {

/// A unique directory for temporary files. The directory is deleted when
/// the `TempDirectory` is destroyed.
class TempDirectory final {
 public:
  /// @brief Create the directory with permissions=700
  /// @throws std::runtime_error
  TempDirectory();

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
