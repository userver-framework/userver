#include <userver/concurrent/impl/asymmetric_fence.hpp>

#ifdef __linux__

#include <linux/membarrier.h>
#include <sys/syscall.h>
#include <unistd.h>

#include <atomic>
#include <cstdint>

#include <userver/compiler/impl/constexpr.hpp>
#include <userver/engine/task/task_processor_fwd.hpp>
#include <userver/utils/assert.hpp>
#include <utils/impl/assert_extra.hpp>

USERVER_NAMESPACE_BEGIN

namespace concurrent::impl {

namespace {

enum class MembarrierRegistrationStatus : std::uint8_t {
  kNotCheckedYet = 0,
  kRegistered,
  kUnsupported,
};

thread_local USERVER_IMPL_CONSTINIT auto thread_registration_status =
    MembarrierRegistrationStatus::kNotCheckedYet;

bool IsMembarrierAvailable() noexcept {
  const auto ret = syscall(__NR_membarrier, MEMBARRIER_CMD_QUERY, 0);
  return ret != -1 && (ret & MEMBARRIER_CMD_PRIVATE_EXPEDITED) != 0;
}

bool IsMembarrierAvailableCached() noexcept {
  static const bool cache = IsMembarrierAvailable();
  return cache;
}

bool TryRegisterThread() noexcept {
  if (!IsMembarrierAvailableCached()) {
    return false;
  }

  const auto ret =
      syscall(__NR_membarrier, MEMBARRIER_CMD_REGISTER_PRIVATE_EXPEDITED, 0);
  if (ret != 0) {
    utils::impl::AbortWithStacktrace("membarrier init failed");
  }
  return true;
}

bool TryRegisterThreadCached() noexcept {
  const auto old_status = thread_registration_status;
  if (old_status == MembarrierRegistrationStatus::kRegistered) {
    return true;
  }
  AsymmetricThreadFenceForceRegisterThread();
  return thread_registration_status ==
         MembarrierRegistrationStatus::kRegistered;
}

const bool task_processor_hook_registration =
    (engine::RegisterThreadStartedHook(
         AsymmetricThreadFenceForceRegisterThread),
     false);

}  // namespace

void AsymmetricThreadFenceLight() noexcept {
  (void)task_processor_hook_registration;  // odr-use

  // NOLINTNEXTLINE(hicpp-no-assembler)
  asm volatile("" : : : "memory");

  if (!TryRegisterThreadCached()) {
    std::atomic_thread_fence(std::memory_order_seq_cst);
  }
}

void AsymmetricThreadFenceHeavy() noexcept {
  if (IsMembarrierAvailableCached()) {
    const auto ret =
        syscall(__NR_membarrier, MEMBARRIER_CMD_PRIVATE_EXPEDITED, 0);
    if (ret != 0) {
      utils::impl::AbortWithStacktrace("membarrier usage failed");
    }
  } else {
    std::atomic_thread_fence(std::memory_order_seq_cst);
  }
}

void AsymmetricThreadFenceForceRegisterThread() noexcept {
  if (__builtin_expect(thread_registration_status ==
                           MembarrierRegistrationStatus::kNotCheckedYet,
                       false)) {
    thread_registration_status =
        TryRegisterThread() ? MembarrierRegistrationStatus::kRegistered
                            : MembarrierRegistrationStatus::kUnsupported;
  }
}

}  // namespace concurrent::impl

USERVER_NAMESPACE_END

#else

#include <atomic>

USERVER_NAMESPACE_BEGIN

namespace concurrent::impl {

void AsymmetricThreadFenceLight() noexcept {
  std::atomic_thread_fence(std::memory_order_seq_cst);
}

void AsymmetricThreadFenceHeavy() noexcept {
  std::atomic_thread_fence(std::memory_order_seq_cst);
}

void AsymmetricThreadFenceForceRegisterThread() noexcept {}

}  // namespace concurrent::impl

USERVER_NAMESPACE_END

#endif
