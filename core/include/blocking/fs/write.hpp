#pragma once

#include <string>

namespace blocking {
namespace fs {

void RewriteFileContents(const std::string& path, std::string contents);

/* open()+fsync()+close(), can be used both on regular files and directories */
void SyncDirectoryContents(const std::string& path);

void Rename(const std::string& source, const std::string& destination);

}  // namespace fs
}  // namespace blocking
