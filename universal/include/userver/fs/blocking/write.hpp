#pragma once

/// @file userver/fs/blocking/write.hpp
/// @brief Functions for synchronous (blocking) file write operations
/// @ingroup userver_universal

#include <filesystem>
#include <string_view>

#include <userver/fs/blocking/impl/permissions.hpp>
#include <userver/utils/zstring_view.hpp>

USERVER_NAMESPACE_BEGIN

namespace fs::blocking {

/// @brief Create directory and all necessary parent elements. Condition when
/// path already exists and is a directory treated as "success" and no exception
/// is thrown.
/// @param path directory to create
/// @param perms new directory permissions, default=0755
/// @throws std::runtime_error if an error occurred while creating directories
void CreateDirectories(std::string_view path, std::filesystem::perms perms);

/// @overload
/// Accepts `boost::filesystem::perms`.
/// @deprecated use the std::filesystem overload
template <impl::LegacyPermissions Permissions>
void CreateDirectories(std::string_view path, Permissions perms) {
    fs::blocking::CreateDirectories(path, impl::ToStdPermissions(perms));
}

/// @overload
void CreateDirectories(std::string_view path);

/// @brief Rewrite file contents synchronously
/// @param path file to rewrite
/// @param contents new file contents
/// @throws std::runtime_error if failed to overwrite
/// @see fs::RewriteFileContents
void RewriteFileContents(utils::zstring_view path, std::string_view contents);

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
void RewriteFileContentsFSync(utils::zstring_view path, std::string_view contents);

/// @brief flushes directory contents on disk using sync(2)
/// @param path directory to flush
void SyncDirectoryContents(utils::zstring_view path);

/// @brief Renames existing file synchronously
/// @param source path to move from
/// @param destination path to move to
/// @throws std::runtime_error
void Rename(utils::zstring_view source, utils::zstring_view destination);

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
/// @param options how to apply permissions to the temporary file
/// @throws std::runtime_error
void RewriteFileContentsAtomically(
    utils::zstring_view path,
    std::string_view contents,
    std::filesystem::perms perms,
    std::filesystem::perm_options options = std::filesystem::perm_options::replace
);

/// @overload
/// Accepts `boost::filesystem::perms` with combined permission and operation flags.
/// @deprecated use the std::filesystem overload
template <impl::LegacyPermissions Permissions>
void RewriteFileContentsAtomically(utils::zstring_view path, std::string_view contents, Permissions perms) {
    fs::blocking::RewriteFileContentsAtomically(
        path,
        contents,
        impl::ToStdPermissions(perms),
        impl::ToStdPermOptions(perms)
    );
}

/// @brief Change file mode synchronously
/// @pre The path must not be a symlink.
/// @param path file path to chmod
/// @param perms new file permissions
/// @param options whether to replace, add or remove permissions and whether to follow symlinks
/// @throws std::runtime_error
void Chmod(
    utils::zstring_view path,
    std::filesystem::perms perms,
    std::filesystem::perm_options options = std::filesystem::perm_options::replace
);

/// @overload
/// Accepts `boost::filesystem::perms` with combined permission and operation flags.
/// @deprecated use the std::filesystem overload
template <impl::LegacyPermissions Permissions>
void Chmod(utils::zstring_view path, Permissions perms) {
    fs::blocking::Chmod(path, impl::ToStdPermissions(perms), impl::ToStdPermOptions(perms));
}

/// @brief Remove existing file synchronously
/// @param path file path to chmod
/// @returns true if successfully removed, false if file doesn't exist
/// @throws std::runtime_error
bool RemoveSingleFile(utils::zstring_view path);

}  // namespace fs::blocking

USERVER_NAMESPACE_END
