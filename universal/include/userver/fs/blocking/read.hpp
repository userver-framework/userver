#pragma once

/// @file userver/fs/blocking/read.hpp
/// @brief functions for synchronous (blocking) file read operations

#include <string>

#include <boost/filesystem/operations.hpp>

USERVER_NAMESPACE_BEGIN

/// @brief blocking function and classes to work with filesystem
///
/// Use these with caution as they block current thread.  It is probably OK to
/// use them during startup (on component load), but don't use them after server
/// start in the main TaskProcessor. Use asynchronous alternatives from `fs`
/// namespace instead.
namespace fs::blocking {

/// @brief Reads file contents synchronously
/// @param path file to open
/// @returns file contents
/// @throws std::runtime_error if read fails for any reason (e.g. no such file,
/// read error, etc.),
std::string ReadFileContents(const std::string& path);

/// @brief Checks whether the file exists synchronously
/// @param path file path to check
/// @returns true if file exists, false if file doesn't exist
/// @throws std::runtime_error if something goes wrong (e.g. out of file
/// descriptors)
bool FileExists(const std::string& path);

/// @brief Get file type returned by stat(2) synchronously.
/// @param path file path
/// @throws std::runtime_error if something goes wrong
boost::filesystem::file_type GetFileType(const std::string& path);

}  // namespace fs::blocking

USERVER_NAMESPACE_END
