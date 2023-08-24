#include <dump/internal_helpers_test.hpp>

#include <boost/filesystem.hpp>

#include <dump/dump_locator.hpp>
#include <userver/formats/yaml/serialize.hpp>
#include <userver/fs/blocking/write.hpp>
#include <userver/utils/datetime.hpp>

USERVER_NAMESPACE_BEGIN

namespace dump {

dump::Config ConfigFromYaml(const std::string& yaml_string,
                            const fs::blocking::TempDirectory& dump_root,
                            std::string_view dumper_name) {
  return {std::string{dumper_name},
          {formats::yaml::FromString(yaml_string), {}},
          dump_root.GetPath()};
}

void CreateDumps(const std::vector<std::string>& filenames,
                 const fs::blocking::TempDirectory& dump_root,
                 std::string_view dumper_name) {
  const auto full_directory =
      boost::filesystem::path{dump_root.GetPath()} / std::string{dumper_name};

  fs::blocking::CreateDirectories(full_directory.string());

  for (const auto& filename : filenames) {
    fs::blocking::RewriteFileContents((full_directory / filename).string(),
                                      filename);
  }
}

void CreateDump(std::string_view contents, const Config& config) {
  const auto dump_stats = dump::DumpLocator{config}.RegisterNewDump(
      std::chrono::time_point_cast<TimePoint::duration>(
          utils::datetime::Now()));
  fs::blocking::RewriteFileContents(dump_stats.full_path, contents);
}

std::set<std::string> FilenamesInDirectory(
    const fs::blocking::TempDirectory& dump_root,
    std::string_view dumper_name) {
  const auto full_directory =
      boost::filesystem::path{dump_root.GetPath()} / std::string{dumper_name};
  if (!boost::filesystem::exists(full_directory)) return {};

  std::set<std::string> result;
  for (const auto& file :
       boost::filesystem::directory_iterator{full_directory}) {
    result.insert(file.path().filename().string());
  }
  return result;
}

}  // namespace dump

USERVER_NAMESPACE_END
