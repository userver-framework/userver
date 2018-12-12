#include <async/fs/write.hpp>

#include <boost/filesystem/operations.hpp>

#include <blocking/fs/write.hpp>
#include <engine/async.hpp>

namespace async {
namespace fs {

void RewriteFileContents(engine::TaskProcessor& async_tp,
                         const std::string& path, std::string contents) {
  engine::Async(async_tp, &blocking::fs::RewriteFileContents, path,
                std::move(contents))
      .Get();
}

void SyncDirectoryContents(engine::TaskProcessor& async_tp,
                           const std::string& path) {
  engine::Async(async_tp, &blocking::fs::SyncDirectoryContents, path).Get();
}

void Rename(engine::TaskProcessor& async_tp, const std::string& source,
            const std::string& destination) {
  engine::Async(async_tp, &blocking::fs::Rename, source, destination).Get();
}

void RewriteFileContentsAtomically(engine::TaskProcessor& async_tp,
                                   const std::string& path,
                                   std::string contents) {
  auto tmp_path = path + ".tmp";
  RewriteFileContents(async_tp, tmp_path, std::move(contents));

  boost::filesystem::path file_path(path);
  auto directory_path = file_path.parent_path();

  Rename(async_tp, tmp_path, path);
  SyncDirectoryContents(async_tp, directory_path.string());
}

}  // namespace fs
}  // namespace async
