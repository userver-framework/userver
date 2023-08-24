#include <userver/fs/read.hpp>

#include <userver/engine/async.hpp>
#include <userver/fs/blocking/read.hpp>

USERVER_NAMESPACE_BEGIN

namespace fs {

namespace {

bool IsHiddenFile(const boost::filesystem::path& path) {
  auto name = path.filename().native();
  UASSERT(!name.empty());
  return name != ".." && name != "." && name[0] == '.';
}

std::string GetRelative(std::string_view path, std::string_view dir) {
  UASSERT(dir.size() < path.size());
  auto rel = path.substr(dir.size());
  return std::string{rel};
}

}  // namespace

std::string ReadFileContents(engine::TaskProcessor& async_tp,
                             const std::string& path) {
  return engine::AsyncNoSpan(async_tp, &fs::blocking::ReadFileContents, path)
      .Get();
}

FileInfoWithDataMap ReadRecursiveFilesInfoWithData(
    engine::TaskProcessor& async_tp, const std::string& path,
    utils::Flags<SettingsReadFile> flags) {
  FileInfoWithDataMap data{};
  for (const auto& f : boost::filesystem::recursive_directory_iterator(path)) {
    // only files
    if (f.status().type() != boost::filesystem::regular_file) continue;
    if ((flags & SettingsReadFile::kSkipHidden) && IsHiddenFile(f.path()))
      continue;
    FileInfoWithData info{};
    info.size = boost::filesystem::file_size(f.path());
    info.extension = f.path().extension().string();
    info.data = ReadFileContents(async_tp, f.path().string());
    data[GetRelative(f.path().string(), path)] =
        std::make_shared<const FileInfoWithData>(std::move(info));
  }
  return data;
}

bool FileExists(engine::TaskProcessor& async_tp, const std::string& path) {
  return engine::AsyncNoSpan(async_tp, &fs::blocking::FileExists, path).Get();
}

}  // namespace fs

USERVER_NAMESPACE_END
