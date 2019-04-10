#pragma once

// NOLINTNEXTLINE(modernize-deprecated-headers,hicpp-deprecated-headers)
#include <signal.h>

#include <initializer_list>

namespace utils {

class SignalCatcher {
 public:
  SignalCatcher(std::initializer_list<int> signals);
  // NOLINTNEXTLINE(bugprone-exception-escape)
  ~SignalCatcher() noexcept(false);

  int Catch();

 private:
  sigset_t sigset_{};
  sigset_t old_sigset_{};
};

}  // namespace utils
