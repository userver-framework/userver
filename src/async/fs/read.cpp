#include <async/fs/read.hpp>

#include <blocking/fs/read.hpp>
#include <engine/async.hpp>

namespace async {
namespace fs {

std::string ReadFileContents(engine::TaskProcessor& async_tp,
                             const std::string& path) {
  return engine::Async(async_tp, &blocking::fs::ReadFileContents, path).Get();
}

}  // namespace fs
}  // namespace async
