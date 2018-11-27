#include <blocking/fs/read.hpp>

#include <fstream>
#include <sstream>

namespace blocking {
namespace fs {

std::string ReadFileContents(const std::string& path) {
  std::ifstream ifs(path);
  if (!ifs) {
    throw std::runtime_error("Error opening '" + path + '\'');
  }

  std::ostringstream buffer;
  buffer << ifs.rdbuf();
  return buffer.str();
}

}  // namespace fs
}  // namespace blocking
