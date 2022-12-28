#pragma once

#include <cerrno>
#include <system_error>

#include <fmt/format.h>

#include <userver/utils/fmt_compat.hpp>

USERVER_NAMESPACE_BEGIN

namespace utils {

template <typename Exception, typename ErrorMark, typename Ret, typename Format,
          typename... Args>
Ret CheckSyscallNotEqualsCustomException(Ret ret, ErrorMark mark,
                                         const Format& format,
                                         const Args&... args) {
  if (ret == mark) {
    // avoid losing errno due to message generation
    const auto err_value = errno;
    fmt::memory_buffer msg_buf;
    fmt::format_to(std::back_inserter(msg_buf), "Error while ");
    fmt::format_to(std::back_inserter(msg_buf), fmt::runtime(format), args...);
    msg_buf.push_back('\0');
    throw Exception(std::error_code(err_value, std::system_category()),
                    msg_buf.data());
  }

  return ret;
}

template <typename Exception, typename Ret, typename Format, typename... Args>
Ret CheckSyscallCustomException(Ret ret, const Format& format,
                                const Args&... args) {
  return CheckSyscallNotEqualsCustomException<Exception>(
      ret, static_cast<Ret>(-1), format, args...);
}

template <typename Ret, typename ErrorMark, typename Format, typename... Args>
Ret CheckSyscallNotEquals(Ret ret, ErrorMark mark, const Format& format,
                          const Args&... args) {
  return CheckSyscallNotEqualsCustomException<std::system_error>(
      ret, mark, format, args...);
}

template <typename Ret, typename Format, typename... Args>
Ret CheckSyscall(Ret ret, const Format& format, const Args&... args) {
  return CheckSyscallNotEquals(ret, static_cast<Ret>(-1), format, args...);
}

}  // namespace utils

USERVER_NAMESPACE_END
