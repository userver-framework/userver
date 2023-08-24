#pragma once

// NOLINTNEXTLINE(modernize-deprecated-headers)
#include <signal.h>

#include <initializer_list>

USERVER_NAMESPACE_BEGIN

namespace utils {

class SignalCatcher final {
 public:
  SignalCatcher(std::initializer_list<int> signals);
  ~SignalCatcher() noexcept(false);

  int Catch();

 private:
  sigset_t sigset_{};
  sigset_t old_sigset_{};
};

}  // namespace utils

USERVER_NAMESPACE_END
