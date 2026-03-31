#pragma once

#include <cstdint>
#include <thread>

USERVER_NAMESPACE_BEGIN

namespace compiler {

class RelaxCpu final {
public:
    RelaxCpu() = default;

    // Should be called in spin-wait loops to reduce contention and allow other
    // threads to execute.
    void operator()() noexcept {
        if (++spins_performed_ % kMaxSpins == 0) {
            std::this_thread::yield();
        } else {
            YieldCpu();
        }
    }

    // Does not do `std::this_thread::yield()`, not recommended in production code.
    // Useful in some tests to get as tight timings as possible to detect race conditions.
    static void YieldCpuDebug() { YieldCpu(); }

private:
    static constexpr std::uint32_t kMaxSpins = 16;

    static void YieldCpu() noexcept {
#if defined(__x86_64__) || defined(__amd64__) || defined(__i386__)
        __builtin_ia32_pause();
#elif defined(__aarch64__)
        __asm__ __volatile__("yield");
#else
#error "Unsupported CPU architecture"
#endif
    }

    std::uint32_t spins_performed_{0};
};

}  // namespace compiler

USERVER_NAMESPACE_END
