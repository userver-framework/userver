#pragma once

/// @file userver/fs/read.hpp
/// @brief functions for asyncronous file read operations

#include <string>

#include <userver/engine/task/task_processor_fwd.hpp>

/// @brief filesystem support
USERVER_NAMESPACE_BEGIN

namespace fs {

/// @brief Reads file contents asynchronously
/// @param async_tp TaskProcessor for synchronous waiting
/// @param path file to open
/// @returns file contents
/// @throws std::runtime_error if read fails for any reason (e.g. no such file,
/// read error, etc.),
std::string ReadFileContents(engine::TaskProcessor& async_tp,
                             const std::string& path);

/// @brief Checks whether the file exists asynchronosly
/// @param async_tp TaskProcessor for synchronous waiting
/// @param path file path to check
/// @returns true if file exists, false if file doesn't exist
/// @throws std::runtime_error if something goes wrong (e.g. out of file
/// descriptors)
bool FileExists(engine::TaskProcessor& async_tp, const std::string& path);

}  // namespace fs

USERVER_NAMESPACE_END
