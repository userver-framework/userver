#include <cache/cache_test_helpers.hpp>

#include <formats/yaml/serialize.hpp>
#include <formats/yaml/value_builder.hpp>

namespace cache {

CacheConfigStatic ConfigFromYaml(const std::string& yaml_string,
                                 const std::string& dump_root,
                                 std::string cache_name) {
  formats::yaml::ValueBuilder config_vars(formats::yaml::Type::kObject);
  config_vars["userver-cache-dump-path"] = dump_root;
  components::ComponentConfig source{yaml_config::YamlConfig{
      formats::yaml::FromString(yaml_string), config_vars.ExtractValue()}};
  source.SetName(std::move(cache_name));
  return cache::CacheConfigStatic{source};
}

boost::filesystem::path RandomDumpDirectory() {
  auto result =
      boost::filesystem::temp_directory_path() /
      boost::filesystem::path{"yandex"} /
      boost::filesystem::path{"cache-dump-test"} /
      boost::filesystem::unique_path("%%%%%%%%-%%%%%%%%-%%%%%%%%-%%%%%%%%");
  UASSERT_MSG(!boost::filesystem::exists(result),
              "Unfortunate collision of dump directories");
  return result;
}

void CreateDumps(const std::vector<boost::filesystem::path>& filenames,
                 const boost::filesystem::path& directory,
                 std::string_view cache_name) {
  const auto full_directory = directory / std::string{cache_name};
  fs::blocking::CreateDirectories(full_directory.string());
  for (const auto& filename : filenames) {
    fs::blocking::RewriteFileContents((full_directory / filename).string(),
                                      filename.string());
  }
}

void ClearDumps(const boost::filesystem::path& directory) {
  boost::filesystem::remove_all(directory);
}

std::set<boost::filesystem::path> FilenamesInDirectory(
    const boost::filesystem::path& directory, std::string_view cache_name) {
  const auto full_directory = directory / std::string{cache_name};
  if (!boost::filesystem::exists(full_directory)) return {};

  // Can't use std::unordered_set with boost::filesystem::path :(
  std::set<boost::filesystem::path> result;
  for (const auto& file :
       boost::filesystem::directory_iterator{full_directory}) {
    result.insert(file.path().filename());
  }
  return result;
}

}  // namespace cache
