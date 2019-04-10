#pragma once

#include <string>

#include <boost/filesystem/operations.hpp>

namespace fs {
namespace blocking {

/// @brief Rewrite file contents synchronously
/// @param path file to rewrite
/// @param contents new file contents
/// @throws std::runtime_error if failed to overwrite
/// @see fs::RewriteFileContents
void RewriteFileContents(const std::string& path, const std::string& contents);

/// @brief flushes directory contents on disk using sync(2)
/// @param path directory to flush
void SyncDirectoryContents(const std::string& path);

/// @brief Renames existing file synchronously
/// @param source path to move from
/// @param destination path to move to
/// @throws std::runtime_error
void Rename(const std::string& source, const std::string& destination);

/// @brief Change file mode synchronously
/// @param path file path to chmod
/// @param perms new file permissions
/// @throws std::runtime_error
void Chmod(const std::string& path, boost::filesystem::perms perms);

/// @brief Remove existing file synchronously
/// @param path file path to chmod
/// @returns true if successfully removed, false if file doesn't exist
/// @throws std::runtime_error
bool RemoveSingleFile(const std::string& path);

}  // namespace blocking
}  // namespace fs
