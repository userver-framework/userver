#include <userver/fs/read.hpp>

#include <userver/engine/async.hpp>
#include <userver/fs/blocking/read.hpp>

USERVER_NAMESPACE_BEGIN

namespace fs {

std::string ReadFileContents(engine::TaskProcessor& async_tp,
                             const std::string& path) {
  return engine::AsyncNoSpan(async_tp, &fs::blocking::ReadFileContents, path)
      .Get();
}

bool FileExists(engine::TaskProcessor& async_tp, const std::string& path) {
  return engine::AsyncNoSpan(async_tp, &fs::blocking::FileExists, path).Get();
}

}  // namespace fs

USERVER_NAMESPACE_END
