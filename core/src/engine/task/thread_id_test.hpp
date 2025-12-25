#include <userver/compiler/thread_local.hpp>

USERVER_NAMESPACE_BEGIN

namespace test {

// Using implementation from scratch as the core implementation may be turned into noop under pinning task queues.
inline __attribute__((noinline)) auto GetThreadIdSafe() {
    // Implementation note: this 'asm' and 'noinline' prevent the compiler
    // from caching the TLS address across a coroutine context switch.
    // The general approach is taken from:
    // https://github.com/qemu/qemu/blob/stable-8.2/include/qemu/coroutine-tls.h#L153

    auto id = std::this_thread::get_id();
    auto* ptr = &id;
    // NOLINTNEXTLINE(hicpp-no-assembler)
    asm volatile("" : "+rm"(ptr));
    return *ptr;
}

}  // namespace test

USERVER_NAMESPACE_END
