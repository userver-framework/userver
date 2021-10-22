#pragma once

#include <cerrno>
#include <system_error>

#include <fmt/format.h>

USERVER_NAMESPACE_BEGIN

namespace utils {

template <typename Ret, typename ErrorMark, typename Format, typename... Args>
Ret CheckSyscallNotEquals(Ret ret, ErrorMark mark, const Format& format,
                          const Args&... args) {
  if (ret == mark) {
    // avoid losing errno due to message generation
    const auto err_value = errno;
    fmt::memory_buffer msg_buf;
    fmt::format_to(msg_buf, "Error while ");
    fmt::format_to(msg_buf, format, args...);
    msg_buf.push_back('\0');
    throw std::system_error(err_value, std::generic_category(), msg_buf.data());
  }
  return ret;
}

template <typename Ret, typename Format, typename... Args>
Ret CheckSyscall(Ret ret, const Format& format, const Args&... args) {
  return CheckSyscallNotEquals(ret, static_cast<Ret>(-1), format, args...);
}

}  // namespace utils

USERVER_NAMESPACE_END
