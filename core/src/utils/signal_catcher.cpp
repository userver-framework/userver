#include <utils/signal_catcher.hpp>

#include <userver/utils/assert.hpp>
#include <utils/check_syscall.hpp>

USERVER_NAMESPACE_BEGIN

namespace utils {

SignalCatcher::SignalCatcher(std::initializer_list<int> signals) {
  utils::CheckSyscall(sigemptyset(&sigset_), "initializing signal set");
  for (int signum : signals) {
    utils::CheckSyscall(sigaddset(&sigset_, signum), "adding signal to set");
  }
  utils::CheckSyscall(pthread_sigmask(SIG_BLOCK, &sigset_, &old_sigset_),
                      "blocking signals");
}

SignalCatcher::~SignalCatcher() noexcept(false) {
  utils::CheckSyscall(pthread_sigmask(SIG_SETMASK, &old_sigset_, nullptr),
                      "restoring signal mask");
}

int SignalCatcher::Catch() {
  int signum = -1;
  utils::CheckSyscall(sigwait(&sigset_, &signum), "waiting for signal");
  UASSERT(signum != -1);
  return signum;
}

}  // namespace utils

USERVER_NAMESPACE_END
