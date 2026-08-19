#pragma once

/// @file userver/fs/file_info_with_data.hpp
/// @brief data structures to store file info with load data

#include <filesystem>
#include <memory>
#include <string>
#include <unordered_map>
#include <variant>

USERVER_NAMESPACE_BEGIN

/// @brief filesystem support
namespace fs {

/// @brief Struct file with load data
///
/// `data_or_path` holds either file contents (`std::string`) or a full path to the file
/// (`std::filesystem::path`) when the file is too large to cache in memory.
struct FileInfoWithData {
    std::variant<std::string, std::filesystem::path> data_or_path;
    std::string extension;
};

using FileInfoWithDataConstPtr = std::shared_ptr<const FileInfoWithData>;
using FileInfoWithDataMap = std::unordered_map<std::string, FileInfoWithDataConstPtr>;

}  // namespace fs

USERVER_NAMESPACE_END
