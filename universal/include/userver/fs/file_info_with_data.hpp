#pragma once

/// @file userver/fs/file_info_with_data.hpp
/// @brief data structures to store file info with load data

#include <memory>
#include <string>
#include <unordered_map>

USERVER_NAMESPACE_BEGIN

/// @brief filesystem support
namespace fs {

/// @brief Struct file with load data
struct FileInfoWithData {
    std::string data;
    std::string extension;
};

using FileInfoWithDataConstPtr = std::shared_ptr<const FileInfoWithData>;
using FileInfoWithDataMap = std::unordered_map<std::string, FileInfoWithDataConstPtr>;

}  // namespace fs

USERVER_NAMESPACE_END
