#include <userver/utils/impl/disable_core_dumps.hpp>

#include <sys/resource.h>
#include <cstdlib>
#include <iostream>

#include <utils/check_syscall.hpp>
#include <utils/strerror.hpp>

USERVER_NAMESPACE_BEGIN

namespace utils::impl {

class DisableCoreDumps::Impl final {
 public:
  Impl() {
    utils::CheckSyscall(::getrlimit(RLIMIT_CORE, &old_limits_), "getrlimit");

    auto new_limits = old_limits_;
    new_limits.rlim_cur = 0;
    utils::CheckSyscall(::setrlimit(RLIMIT_CORE, &new_limits), "setrlimit");
  }

  ~Impl() {
    if (::setrlimit(RLIMIT_CORE, &old_limits_) == -1) {
      const auto error_code = errno;
      std::cerr << "Failed to return the core dump limit to defaults: "
                << utils::strerror(error_code) << std::flush;
      std::abort();
    }
  }

 private:
  ::rlimit old_limits_{};
};

DisableCoreDumps::DisableCoreDumps() : impl_(std::make_unique<Impl>()) {}

DisableCoreDumps::~DisableCoreDumps() = default;

bool DisableCoreDumps::IsValid() const noexcept { return impl_ != nullptr; }

void DisableCoreDumps::Invalidate() noexcept { impl_ = nullptr; }

}  // namespace utils::impl

USERVER_NAMESPACE_END
