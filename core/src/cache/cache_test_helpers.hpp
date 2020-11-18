#pragma once

#include <boost/filesystem.hpp>

#include <cache/dumper.hpp>
#include <fs/blocking/write.hpp>
#include <utils/scope_guard.hpp>

#include <utils/mock_now.hpp>

namespace cache {

CacheConfigStatic ConfigFromYaml(const std::string& yaml_string,
                                 const std::string& dump_root,
                                 std::string cache_name);

boost::filesystem::path RandomDumpDirectory();

/// Create files, writing their own filenames into them
void CreateDumps(const std::vector<boost::filesystem::path>& filenames,
                 const boost::filesystem::path& directory,
                 std::string_view cache_name);

/// Remove the temporary dump directory
void ClearDumps(const boost::filesystem::path& directory);

/// @note Returns filenames, not full paths
std::set<boost::filesystem::path> FilenamesInDirectory(
    const boost::filesystem::path& directory, std::string_view cache_name);

}  // namespace cache
