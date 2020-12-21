#include <boost/filesystem.hpp>

#include <utils/check_syscall.hpp>

namespace cache::dump::impl {

std::string PrepareTempFile() {
  std::string directory =
      boost::filesystem::temp_directory_path().string() + "/.XXXXXX";
  utils::CheckSyscallNotEquals(::mkdtemp(directory.data()), nullptr,
                               "::mkdtemp");
  return directory + "/dump-load";
}

}  // namespace cache::dump::impl
