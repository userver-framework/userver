#pragma once

#include <cerrno>
#include <sstream>
#include <string>
#include <system_error>

namespace utils {
namespace impl {

inline void Dump(std::ostringstream&) {}

template <typename Arg, typename... Args>
void Dump(std::ostringstream& os, Arg&& arg, Args&&... args) {
  os << std::forward<Arg>(arg);
  Dump(os, std::forward<Args>(args)...);
}

template <typename... Args>
std::string ToString(Args&&... args) {
  std::ostringstream os;
  Dump(os, std::forward<Args>(args)...);
  return os.str();
}

}  // namespace impl

template <typename... Context>
inline int CheckSyscall(int ret, Context&&... context) {
  if (ret == -1) {
    throw std::system_error(
        errno, std::generic_category(),
        "Error while " + impl::ToString(std::forward<Context>(context)...));
  }
  return ret;
}

}  // namespace utils
