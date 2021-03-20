#include <cache/test_helpers.hpp>

#include <boost/filesystem.hpp>

#include <formats/yaml/serialize.hpp>
#include <formats/yaml/value_builder.hpp>
#include <fs/blocking/write.hpp>

namespace cache {

components::ComponentConfig ConfigFromYaml(const std::string& yaml_string,
                                           const std::string& dump_root,
                                           std::string_view cache_name) {
  formats::yaml::ValueBuilder config_vars(formats::yaml::Type::kObject);
  config_vars["userver-cache-dump-path"] = dump_root;
  components::ComponentConfig config{yaml_config::YamlConfig{
      formats::yaml::FromString(yaml_string), config_vars.ExtractValue()}};
  config.SetName(std::string{cache_name});
  return config;
}

void CreateDumps(const std::vector<std::string>& filenames,
                 const std::string& directory, std::string_view cache_name) {
  const auto full_directory =
      boost::filesystem::path{directory} / std::string{cache_name};

  fs::blocking::CreateDirectories(full_directory.string());

  for (const auto& filename : filenames) {
    fs::blocking::RewriteFileContents((full_directory / filename).string(),
                                      filename);
  }
}

std::set<std::string> FilenamesInDirectory(const std::string& directory,
                                           std::string_view cache_name) {
  const auto full_directory =
      boost::filesystem::path{directory} / std::string{cache_name};
  if (!boost::filesystem::exists(full_directory)) return {};

  std::set<std::string> result;
  for (const auto& file :
       boost::filesystem::directory_iterator{full_directory}) {
    result.insert(file.path().filename().string());
  }
  return result;
}

}  // namespace cache
