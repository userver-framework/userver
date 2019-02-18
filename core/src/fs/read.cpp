#include <fs/read.hpp>

#include <engine/async.hpp>
#include <fs/blocking/read.hpp>

namespace fs {

std::string ReadFileContents(engine::TaskProcessor& async_tp,
                             const std::string& path) {
  return engine::impl::Async(async_tp, &fs::blocking::ReadFileContents, path)
      .Get();
}

bool FileExists(engine::TaskProcessor& async_tp, const std::string& path) {
  return engine::impl::Async(async_tp, &fs::blocking::FileExists, path).Get();
}

}  // namespace fs
