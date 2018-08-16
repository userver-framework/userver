#include <utils/strerror.hpp>

#include <signal.h>

#include <system_error>

namespace utils {

std::string strerror(int return_code) {
  std::error_code ec(return_code, std::system_category());
  return ec.message();
}

std::string strsignal(int signal_num) {
  constexpr int max_signal_num = sizeof(sys_siglist) / sizeof(sys_siglist[0]);
  static_assert(max_signal_num > 1,
                "sys_siglist must be an array, not a pointer");

  if (signal_num >= 0 && signal_num < max_signal_num)
    return sys_siglist[signal_num];

  if (signal_num >= SIGRTMIN && signal_num <= SIGRTMAX)
    return "Real-time signal " + std::to_string(signal_num);

  return "Unknown signal " + std::to_string(signal_num);
}

}  // namespace utils
