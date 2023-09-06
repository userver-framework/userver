#include <userver/fs/read.hpp>

#include <userver/engine/async.hpp>
#include <userver/fs/blocking/read.hpp>
#include <userver/utils/async.hpp>

USERVER_NAMESPACE_BEGIN

namespace fs {

namespace {

bool IsHiddenFile(const boost::filesystem::path& path) {
  auto name = path.filename().native();
  UASSERT(!name.empty());
  return name != ".." && name != "." && name[0] == '.';
}

}  // namespace

std::string GetLexicallyRelative(std::string_view path, std::string_view dir) {
  UASSERT(dir.size() < path.size());
  UASSERT(path.substr(0, dir.size()) == dir);
  auto rel = path.substr(dir.size());
  return std::string{rel};
}

std::string ReadFileContents(engine::TaskProcessor& async_tp,
                             const std::string& path) {
  return engine::AsyncNoSpan(async_tp, &fs::blocking::ReadFileContents, path)
      .Get();
}

FileInfoWithDataMap ReadRecursiveFilesInfoWithData(
    engine::TaskProcessor& async_tp, const std::string& path,
    utils::Flags<SettingsReadFile> flags) {
  FileInfoWithDataMap data{};
  for (auto it =
           utils::Async(
               async_tp, "init",
               [&path] {
                 return boost::filesystem::recursive_directory_iterator(path);
               })
               .Get();
       it != boost::filesystem::recursive_directory_iterator();
       utils::Async(async_tp, "next", [&it] { ++it; }).Get()) {
    // only files
    if (it->status().type() != boost::filesystem::regular_file) continue;
    if ((flags & SettingsReadFile::kSkipHidden) && IsHiddenFile(it->path()))
      continue;
    FileInfoWithData info{};
    info.extension = it->path().extension().string();
    info.data = ReadFileContents(async_tp, it->path().string());
    data[GetLexicallyRelative(it->path().string(), path)] =
        std::make_shared<const FileInfoWithData>(std::move(info));
  }
  return data;
}

bool FileExists(engine::TaskProcessor& async_tp, const std::string& path) {
  return engine::AsyncNoSpan(async_tp, &fs::blocking::FileExists, path).Get();
}

}  // namespace fs

USERVER_NAMESPACE_END
