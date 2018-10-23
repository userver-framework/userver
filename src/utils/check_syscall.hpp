#pragma once

#include <cerrno>
#include <sstream>
#include <string>
#include <system_error>

namespace utils {
namespace impl {

template <typename... Args>
std::string ToString(const Args&... args) {
  std::ostringstream os;
  (os << ... << args);
  return os.str();
}

}  // namespace impl

template <typename... Context>
inline int CheckSyscall(int ret, const Context&... context) {
  if (ret == -1) {
    // avoid losing errno due to message generation
    const auto err_value = errno;
    throw std::system_error(err_value, std::generic_category(),
                            "Error while " + impl::ToString(context...));
  }
  return ret;
}

}  // namespace utils
