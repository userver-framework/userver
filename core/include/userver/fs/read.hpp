#pragma once

/// @file userver/fs/read.hpp
/// @brief functions for asynchronous file read operations

#include <memory>
#include <string>
#include <unordered_map>

#include <userver/engine/task/task_processor_fwd.hpp>
#include <userver/utils/flags.hpp>

USERVER_NAMESPACE_BEGIN

/// @brief filesystem support
namespace fs {

/// @brief Struct file with load data
struct FileInfoWithData {
  std::string data;
  std::string extension;
  size_t size;
};

using FileInfoWithDataConstPtr = std::shared_ptr<const FileInfoWithData>;
using FileInfoWithDataMap =
    std::unordered_map<std::string, FileInfoWithDataConstPtr>;

enum class SettingsReadFile {
  kNone = 0,
  /// Skip hidden files,
  kSkipHidden = 1 << 0,
};

/// @brief Returns files from recursively traversed directory
/// @param async_tp TaskProcessor for synchronous waiting
/// @param path to directory to traverse recursively
/// @param flags settings read files
/// @returns map with relative to `path` filepaths and file info
/// @throws std::runtime_error if read fails for any reason (e.g. no such file,
/// read error, etc.),
FileInfoWithDataMap ReadRecursiveFilesInfoWithData(
    engine::TaskProcessor& async_tp, const std::string& path,
    utils::Flags<SettingsReadFile> flags = {SettingsReadFile::kSkipHidden});

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
