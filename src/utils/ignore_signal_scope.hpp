#pragma once

#include <signal.h>

namespace utils {

class IgnoreSignalScope {
 public:
  explicit IgnoreSignalScope(int signal);
  ~IgnoreSignalScope() noexcept(false);

 private:
  int signal_;
  struct sigaction old_action_;
};

}  // namespace utils
