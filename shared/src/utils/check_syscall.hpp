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

template <typename Ret, typename ErrorMark, typename... Context>
Ret CheckSyscallNotEquals(Ret ret, ErrorMark mark, const Context&... context) {
  if (ret == mark) {
    // avoid losing errno due to message generation
    const auto err_value = errno;
    throw std::system_error(err_value, std::generic_category(),
                            "Error while " + impl::ToString(context...));
  }
  return ret;
}

template <typename Ret, typename... Context>
Ret CheckSyscall(Ret ret, const Context&... context) {
  return CheckSyscallNotEquals(ret, static_cast<Ret>(-1), context...);
}

}  // namespace utils
