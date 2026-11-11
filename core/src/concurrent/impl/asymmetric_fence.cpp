#include <userver/concurrent/impl/asymmetric_fence.hpp>

#if defined(__linux__) && __has_include(<linux/membarrier.h>)

#include <linux/membarrier.h>
#include <sys/syscall.h>
#include <unistd.h>

#include <atomic>
#include <cstdint>

#include <userver/engine/task/current_task.hpp>
#include <userver/engine/task/task_processor_fwd.hpp>
#include <userver/utils/assert.hpp>

#include <concurrent/impl/asymmetric_fence_coalescer.hpp>
#include <engine/ev/thread_pool.hpp>
#include <engine/task/task_processor.hpp>

USERVER_NAMESPACE_BEGIN

namespace concurrent::impl {

namespace {

// These constants are not available on the host architecture pre-Linux 4.14.
// Still, they may be supported by the target Linux kernel.
constexpr int kMembarrierCmdPrivateExpedited = 1 << 3;
constexpr int kMembarrierCmdRegisterPrivateExpedited = 1 << 4;

enum class MembarrierRegistrationStatus : std::uint8_t {
    kNotCheckedYet = 0,
    kRegistered,
    kUnsupported,
};

thread_local constinit auto thread_registration_status = MembarrierRegistrationStatus::kNotCheckedYet;

// The union avoids destroying the coalescer: global RCU objects may still issue
// heavy fences during static destruction. constinit avoids initialization order issues.
union ImmortalFenceCoalescer {
    constexpr ImmortalFenceCoalescer()
        : coalescer{}
    {}
    ~ImmortalFenceCoalescer() {}

    AsymmetricFenceCoalescer coalescer;
};

constinit ImmortalFenceCoalescer heavy_fence;

void PerformMembarrier() noexcept {
    const auto ret = syscall(__NR_membarrier, kMembarrierCmdPrivateExpedited, 0);
    if (ret != 0) {
        utils::AbortWithStacktrace("membarrier usage failed");
    }
}

void PerformHeavyFence() noexcept {
    if (!engine::current_task::IsTaskProcessorThread()) {
        heavy_fence.coalescer.Perform(PerformMembarrier);
        return;
    }
    if (heavy_fence.coalescer.TryPerform(PerformMembarrier)) {
        return;
    }

    heavy_fence.coalescer.Enqueue();
    auto& ev_thread = engine::current_task::GetTaskProcessor().EventThreadPool().GetFirstThread();
    ev_thread.RunInEvLoopSync([]() noexcept { heavy_fence.coalescer.PerformPending(PerformMembarrier); });
    // The completion event carries the delegated barrier's effects back to this task.
    // In the P1202 model, this SC fence follows the delegated heavy fence in the SC
    // order. On x86_64, it also provides a full barrier on the CPU where the task
    // resumed, even if it migrated. It does not replace the event's release/acquire
    // synchronization.
    std::atomic_thread_fence(std::memory_order_seq_cst);
}

bool IsMembarrierAvailable() noexcept {
    const auto ret = syscall(__NR_membarrier, MEMBARRIER_CMD_QUERY, 0);
    return ret != -1 && (ret & kMembarrierCmdPrivateExpedited) != 0;
}

bool IsMembarrierAvailableCached() noexcept {
    static const bool kCache = IsMembarrierAvailable();
    return kCache;
}

bool TryRegisterThread() noexcept {
    if (!IsMembarrierAvailableCached()) {
        return false;
    }

    const auto ret = syscall(__NR_membarrier, kMembarrierCmdRegisterPrivateExpedited, 0);
    if (ret != 0) {
        utils::AbortWithStacktrace("membarrier init failed");
    }
    return true;
}

bool TryRegisterThreadCached() noexcept {
    const auto old_status = thread_registration_status;
    if (old_status == MembarrierRegistrationStatus::kRegistered) {
        return true;
    }
    AsymmetricThreadFenceForceRegisterThread();
    return thread_registration_status == MembarrierRegistrationStatus::kRegistered;
}

const bool task_processor_hook_registration =
    (engine::RegisterThreadStartedHook(AsymmetricThreadFenceForceRegisterThread), false);

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
    if (TryRegisterThreadCached()) {
        PerformHeavyFence();
    } else {
        std::atomic_thread_fence(std::memory_order_seq_cst);
    }
}

void AsymmetricThreadFenceForceRegisterThread() noexcept {
    if (__builtin_expect(thread_registration_status == MembarrierRegistrationStatus::kNotCheckedYet, false)) {
        thread_registration_status =
            TryRegisterThread()
                ? MembarrierRegistrationStatus::kRegistered
                : MembarrierRegistrationStatus::kUnsupported;
    }
}

}  // namespace concurrent::impl

USERVER_NAMESPACE_END

#else

#include <atomic>

USERVER_NAMESPACE_BEGIN

namespace concurrent::impl {

void AsymmetricThreadFenceLight() noexcept { std::atomic_thread_fence(std::memory_order_seq_cst); }

void AsymmetricThreadFenceHeavy() noexcept { std::atomic_thread_fence(std::memory_order_seq_cst); }

void AsymmetricThreadFenceForceRegisterThread() noexcept {}

}  // namespace concurrent::impl

USERVER_NAMESPACE_END

#endif
