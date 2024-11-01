#pragma once

/// @file userver/fs/blocking/write.hpp
/// @brief Functions for synchronous (blocking) file write operations
/// @ingroup userver_universal

#include <string>
#include <string_view>

#include <userver/utils/boost_filesystem_file_status.hpp>

USERVER_NAMESPACE_BEGIN

namespace fs::blocking {

/// @brief Create directory and all necessary parent elements. Condition when
/// path already exists and is a directory treated as "success" and no exception
/// is thrown.
/// @param path directory to create
/// @param perms new directory permissions, default=0755
/// @throws std::runtime_error if an error occurred while creating directories
void CreateDirectories(std::string_view path, boost::filesystem::perms perms);

/// @overload
void CreateDirectories(std::string_view path);

/// @brief Rewrite file contents synchronously
/// @param path file to rewrite
/// @param contents new file contents
/// @throws std::runtime_error if failed to overwrite
/// @see fs::RewriteFileContents
void RewriteFileContents(const std::string& path, std::string_view contents);

/// @brief Rewrite file contents synchronously and call `fsync`
///
/// Blocks until the file is actually written to disk. This does not typically
/// speed up the write operation, but is required for the atomic file write
/// technique.
///
/// This function alone does not implement atomic file writes. If you need them,
/// consider using:
///
/// @see fs::RewriteFileContentsAtomically
/// @see fs::blocking::RewriteFileContentsAtomically
///
/// @param path file to rewrite
/// @param contents new file contents
/// @throws std::runtime_error if failed to overwrite
void RewriteFileContentsFSync(const std::string& path, std::string_view contents);

/// @brief flushes directory contents on disk using sync(2)
/// @param path directory to flush
void SyncDirectoryContents(const std::string& path);

/// @brief Renames existing file synchronously
/// @param source path to move from
/// @param destination path to move to
/// @throws std::runtime_error
void Rename(const std::string& source, const std::string& destination);

/// @brief Rewrite file contents atomically
///
/// Writes contents to a temporary file in the same directory,
/// then atomically replaces the destination file with the temporary file.
/// Effectively does write()+sync()+rename()+sync(directory).
/// It does both sync(2) for file and on the directory, so after successful
/// return the file will persist on the filesystem.
///
/// @param path file path to rewrite
/// @param contents new file contents
/// @param perms new file permissions
/// @throws std::runtime_error
void RewriteFileContentsAtomically(const std::string& path, std::string_view contents, boost::filesystem::perms perms);

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

USERVER_NAMESPACE_END
