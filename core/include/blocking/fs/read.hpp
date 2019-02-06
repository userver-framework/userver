#pragma once

#include <string>

#include <boost/filesystem/operations.hpp>

namespace blocking {
namespace fs {

/* Read contents of file in fs. On error throws an exception. */
std::string ReadFileContents(const std::string& path);

/* Checks whether the file exists. It doesn't check whether the file is
 * readable.
 */
bool FileExists(const std::string& path);

/* Get file type returned by stat(2). If the file doesn't exist,
 * returns file_type::status_error.
 */
boost::filesystem::file_type GetFileType(const std::string& path);

}  // namespace fs
}  // namespace blocking
