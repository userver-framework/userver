#include <utils/strerror.hpp>

// NOLINTNEXTLINE(modernize-deprecated-headers)
#include <signal.h>
// NOLINTNEXTLINE(modernize-deprecated-headers)
#include <string.h>

#include <system_error>

USERVER_NAMESPACE_BEGIN

namespace utils {

namespace {

// SFINAE on sigdescr_np
template <class Int>
auto try_sigdescr_np(Int signal_num) noexcept
    -> decltype(sigdescr_np(signal_num)) {
  return sigdescr_np(signal_num);
}

template <class... Args>
const char* try_sigdescr_np(Args...) noexcept {
#if ((__GLIBC__ * 100 + __GLIBC_MINOR__) >= 232)
  static_assert(sizeof...(Args) && false,
                "This version of glibc is known to have sigdescr_np. Error in "
                "function detection");
#endif
  return nullptr;
}

}  // namespace

std::string strerror(int return_code) {
  std::error_code ec(return_code, std::system_category());
  return ec.message();
}

std::string strsignal(int signal_num) {
  if (const auto* descr = utils::try_sigdescr_np(signal_num); descr) {
    return descr;
  }

#ifdef SIGRTMIN
  if (signal_num >= SIGRTMIN && signal_num <= SIGRTMAX)
    return "Real-time signal " + std::to_string(signal_num);
#endif

  return "Signal " + std::to_string(signal_num);
}

}  // namespace utils

USERVER_NAMESPACE_END
