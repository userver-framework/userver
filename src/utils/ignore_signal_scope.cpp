#include "ignore_signal_scope.hpp"

#include <cstring>

#include "check_syscall.hpp"
#include <utils/strerror.hpp>

namespace utils {

IgnoreSignalScope::IgnoreSignalScope(int signal) : signal_(signal) {
  struct sigaction action;
  memset(&action, 0, sizeof(action));
  action.sa_handler = SIG_IGN;
  utils::CheckSyscall(
      sigaction(signal_, &action, &old_action_),
      "setting ignore handler for " + utils::strsignal(signal_));
}

IgnoreSignalScope::~IgnoreSignalScope() noexcept(false) {
  utils::CheckSyscall(
      sigaction(signal_, &old_action_, nullptr),
      "restoring " + utils::strsignal(signal_) + " handler");
}

}  // namespace utils
