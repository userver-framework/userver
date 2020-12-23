#pragma once

/// @file fs/blocking/write.hpp
/// @brief Functions for synchronous (blocking) file write operations

#include <string>
#include <string_view>

#include <boost/filesystem/operations.hpp>

namespace fs::blocking {

/// @brief Create directory and all necessary parent elements. Condition when
/// path already exists and is a directory treated as "success" and no exception
/// is thrown.
/// @param path directory to create
/// @throws std::runtime_error if an error occured while creating directories
/// while creating directories
void CreateDirectories(const std::string& path);

/// @brief Rewrite file contents synchronously
/// @param path file to rewrite
/// @param contents new file contents
/// @throws std::runtime_error if failed to overwrite
/// @see fs::RewriteFileContents
void RewriteFileContents(const std::string& path, std::string_view contents);

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

}  // namespace fs::blocking
