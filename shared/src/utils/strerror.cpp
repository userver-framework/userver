#include <utils/strerror.hpp>

// NOLINTNEXTLINE(modernize-deprecated-headers)
#include <signal.h>
// NOLINTNEXTLINE(modernize-deprecated-headers)
#include <string.h>

#include <system_error>

USERVER_NAMESPACE_BEGIN

namespace utils {

std::string strerror(int return_code) {
  std::error_code ec(return_code, std::system_category());
  return ec.message();
}

std::string strsignal(int signal_num) {
#if ((__GLIBC__ * 100 + __GLIBC_MINOR__) >= 232)
  if (const auto* descr = ::sigdescr_np(signal_num); descr) {
    return descr;
  }
#else
  constexpr int max_signal_num = sizeof(sys_siglist) / sizeof(sys_siglist[0]);
  static_assert(max_signal_num > 1,
                "sys_siglist must be an array, not a pointer");

  if (signal_num >= 0 && signal_num < max_signal_num)
    return sys_siglist[signal_num];
#endif

#ifdef SIGRTMIN
  if (signal_num >= SIGRTMIN && signal_num <= SIGRTMAX)
    return "Real-time signal " + std::to_string(signal_num);
#endif

  return "Unknown signal " + std::to_string(signal_num);
}

}  // namespace utils

USERVER_NAMESPACE_END
