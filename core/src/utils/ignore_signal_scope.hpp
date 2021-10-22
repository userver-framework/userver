#pragma once

// NOLINTNEXTLINE(modernize-deprecated-headers)
#include <signal.h>

USERVER_NAMESPACE_BEGIN

namespace utils {

class IgnoreSignalScope final {
 public:
  explicit IgnoreSignalScope(int signal);
  // NOLINTNEXTLINE(bugprone-exception-escape)
  ~IgnoreSignalScope() noexcept(false);

 private:
  int signal_{0};
  struct sigaction old_action_ {};
};

}  // namespace utils

USERVER_NAMESPACE_END
