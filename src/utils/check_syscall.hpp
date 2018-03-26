#pragma once

#include <cerrno>
#include <string>
#include <system_error>

namespace utils {

inline int CheckSyscall(int ret, const std::string& context) {
  if (ret == -1) {
    throw std::system_error(errno, std::generic_category(),
                            "Error while " + context);
  }
  return ret;
}

}  // namespace utils
