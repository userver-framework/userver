#pragma once

/// @file userver/fs/write.hpp
/// @brief filesystem write functions

#include <filesystem>
#include <string>
#include <string_view>

#include <userver/engine/task/task_processor_fwd.hpp>
#include <userver/fs/blocking/impl/permissions.hpp>

USERVER_NAMESPACE_BEGIN

namespace fs {

/// @{
/// @brief Create directory and all necessary parent elements. Condition when
/// path already exists and is a directory treated as "success" and no exception
/// is thrown.
/// @param async_tp TaskProcessor for synchronous waiting
/// @param path directory to create
/// @param perms permissions to set for the created directories
/// @throws std::runtime_error if there was an error while
/// creating directories
void CreateDirectories(engine::TaskProcessor& async_tp, std::string_view path, std::filesystem::perms perms);

/// @overload
/// Accepts `boost::filesystem::perms`.
/// @deprecated use the std::filesystem overload
template <blocking::impl::LegacyPermissions Permissions>
void CreateDirectories(engine::TaskProcessor& async_tp, std::string_view path, Permissions perms) {
    fs::CreateDirectories(async_tp, path, blocking::impl::ToStdPermissions(perms));
}

void CreateDirectories(engine::TaskProcessor& async_tp, std::string_view path);
/// @}

/// @brief Rewrite file contents asynchronously
/// It doesn't provide strict atomic guarantees. If you need them, use
/// `fs::RewriteFileContentsAtomically`.
/// @param async_tp TaskProcessor for synchronous waiting
/// @param path file to rewrite
/// @param contents new file contents
/// @throws std::runtime_error if failed to overwrite
void RewriteFileContents(engine::TaskProcessor& async_tp, const std::string& path, std::string_view contents);

/// @brief Rewrite file contents asynchronously
/// It doesn't provide strict atomic guarantees. If you need them, use
/// `fs::RewriteFileContentsAtomically`.
/// @param path file to rewrite
/// @param contents new file contents
/// @throws std::runtime_error if failed to overwrite
void RewriteFileContents(const std::string& path, std::string_view contents);

/// @brief Renames existing file
/// @param async_tp TaskProcessor for synchronous waiting
/// @param source path to move from
/// @param destination path to move to
/// @throws std::runtime_error
void Rename(engine::TaskProcessor& async_tp, const std::string& source, const std::string& destination);

/// @brief Rewrite file contents atomically
/// Write contents to temporary file in the same directory,
/// then atomically replaces the destination file with the temporary file.
/// Effectively does write()+sync()+rename()+sync(directory).
/// It does both sync(2) for file and on the directory, so after successful
/// return the file must persist on the filesystem.
/// @param async_tp TaskProcessor for synchronous waiting
/// @param path file path to rewrite
/// @param contents new file contents
/// @param perms new file permissions
/// @param options how to apply permissions
/// @throws std::runtime_error
void RewriteFileContentsAtomically(
    engine::TaskProcessor& async_tp,
    const std::string& path,
    std::string_view contents,
    std::filesystem::perms perms,
    std::filesystem::perm_options options = std::filesystem::perm_options::replace
);

/// @overload
/// Accepts `boost::filesystem::perms` with combined permission and operation flags.
/// @deprecated use the std::filesystem overload
template <blocking::impl::LegacyPermissions Permissions>
void RewriteFileContentsAtomically(
    engine::TaskProcessor& async_tp,
    const std::string& path,
    std::string_view contents,
    Permissions perms
) {
    fs::RewriteFileContentsAtomically(
        async_tp,
        path,
        contents,
        blocking::impl::ToStdPermissions(perms),
        blocking::impl::ToStdPermOptions(perms)
    );
}

/// @brief Change file mode
/// @pre The path must not be a symlink.
/// @param async_tp TaskProcessor for synchronous waiting
/// @param path file path to chmod
/// @param perms new file permissions
/// @param options how to apply permissions
/// @throws std::runtime_error
void Chmod(
    engine::TaskProcessor& async_tp,
    const std::string& path,
    std::filesystem::perms perms,
    std::filesystem::perm_options options = std::filesystem::perm_options::replace
);

/// @overload
/// Accepts `boost::filesystem::perms` with combined permission and operation flags.
/// @deprecated use the std::filesystem overload
template <blocking::impl::LegacyPermissions Permissions>
void Chmod(engine::TaskProcessor& async_tp, const std::string& path, Permissions perms) {
    fs::Chmod(async_tp, path, blocking::impl::ToStdPermissions(perms), blocking::impl::ToStdPermOptions(perms));
}

/// @brief Remove existing file
/// @param async_tp TaskProcessor for synchronous waiting
/// @param path file path to chmod
/// @returns true if successfully removed, false if file doesn't exist
/// @throws std::runtime_error
bool RemoveSingleFile(engine::TaskProcessor& async_tp, const std::string& path);

}  // namespace fs

USERVER_NAMESPACE_END
