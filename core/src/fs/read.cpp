#include <userver/fs/read.hpp>

#include <userver/engine/async.hpp>
#include <userver/fs/blocking/read.hpp>

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
