#pragma once

#include <string>

#include <boost/filesystem/operations.hpp>

namespace blocking {
namespace fs {

void RewriteFileContents(const std::string& path, std::string contents);

/* open()+fsync()+close(), can be used both on regular files and directories */
void SyncDirectoryContents(const std::string& path);

void Rename(const std::string& source, const std::string& destination);

void Chmod(const std::string& path, boost::filesystem::perms perms);

}  // namespace fs
}  // namespace blocking
