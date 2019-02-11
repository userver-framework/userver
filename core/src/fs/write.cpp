#include <fs/write.hpp>

#include <engine/async.hpp>
#include <fs/blocking/write.hpp>

namespace fs {

void RewriteFileContents(engine::TaskProcessor& async_tp,
                         const std::string& path, std::string contents) {
  engine::Async(async_tp, &fs::blocking::RewriteFileContents, path,
                std::move(contents))
      .Get();
}

void SyncDirectoryContents(engine::TaskProcessor& async_tp,
                           const std::string& path) {
  engine::Async(async_tp, &fs::blocking::SyncDirectoryContents, path).Get();
}

void Rename(engine::TaskProcessor& async_tp, const std::string& source,
            const std::string& destination) {
  engine::Async(async_tp, &fs::blocking::Rename, source, destination).Get();
}

void Chmod(engine::TaskProcessor& async_tp, const std::string& path,
           boost::filesystem::perms perms) {
  engine::Async(async_tp, &fs::blocking::Chmod, path, perms).Get();
}

void RewriteFileContentsAtomically(engine::TaskProcessor& async_tp,
                                   const std::string& path,
                                   std::string contents,
                                   boost::filesystem::perms perms) {
  auto tmp_path = path + ".tmp";
  RewriteFileContents(async_tp, tmp_path, std::move(contents));

  boost::filesystem::path file_path(path);
  auto directory_path = file_path.parent_path();

  Rename(async_tp, tmp_path, path);
  SyncDirectoryContents(async_tp, directory_path.string());

  Chmod(async_tp, path, perms);
}

bool RemoveSingleFile(engine::TaskProcessor& async_tp,
                      const std::string& path) {
  return engine::Async(async_tp, &fs::blocking::RemoveSingleFile, path).Get();
}

}  // namespace fs
