#include "ignore_signal_scope.hpp"

#include <cstring>

#include <utils/check_syscall.hpp>
#include <utils/strerror.hpp>

USERVER_NAMESPACE_BEGIN

namespace utils {

IgnoreSignalScope::IgnoreSignalScope(int signal) : signal_(signal) {
  struct sigaction action {};
  memset(&action, 0, sizeof(action));
  // SIG_IGN might be a macro
  // NOLINTNEXTLINE(cppcoreguidelines-pro-type-cstyle-cast)
  action.sa_handler = SIG_IGN;
  utils::CheckSyscall(sigaction(signal_, &action, &old_action_),
                      "setting ignore handler for {}",
                      utils::strsignal(signal_));
}

IgnoreSignalScope::~IgnoreSignalScope() noexcept(false) {
  utils::CheckSyscall(sigaction(signal_, &old_action_, nullptr),
                      "restoring {} handler", utils::strsignal(signal_));
}

}  // namespace utils

USERVER_NAMESPACE_END
