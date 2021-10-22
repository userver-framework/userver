#include <userver/fs/blocking/read.hpp>

#include <fstream>
#include <sstream>

#include <boost/filesystem/operations.hpp>

USERVER_NAMESPACE_BEGIN

namespace fs::blocking {

std::string ReadFileContents(const std::string& path) {
  std::ifstream ifs(path);
  if (!ifs) {
    throw std::runtime_error("Error opening '" + path + '\'');
  }

  std::ostringstream buffer;
  buffer << ifs.rdbuf();
  return buffer.str();
}

bool FileExists(const std::string& path) {
  return boost::filesystem::exists(path);
}

boost::filesystem::file_type GetFileType(const std::string& path) {
  return boost::filesystem::status(path).type();
}

}  // namespace fs::blocking

USERVER_NAMESPACE_END
