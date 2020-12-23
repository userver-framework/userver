#pragma once

#include <set>

#include <cache/cache_config.hpp>

namespace cache {

CacheConfigStatic ConfigFromYaml(const std::string& yaml_string,
                                 const std::string& dump_root,
                                 std::string_view cache_name);

/// Create files, writing their own filenames into them
void CreateDumps(const std::vector<std::string>& filenames,
                 const std::string& directory, std::string_view cache_name);

/// @note Returns filenames, not full paths
std::set<std::string> FilenamesInDirectory(const std::string& directory,
                                           std::string_view cache_name);

}  // namespace cache
