#pragma once

#include <memory>

#include <coroutines/coroutine.hpp>

#include <userver/utils/fast_pimpl.hpp>

USERVER_NAMESPACE_BEGIN

namespace engine::impl {
class TaskContext;
class CountedCoroutinePtr;
}  // namespace engine::impl

namespace engine::coro {

class StackUsageMonitor final {
public:
    explicit StackUsageMonitor(std::size_t coro_stack_size);
    ~StackUsageMonitor();

    void Start();
    void Stop();

    void Register(const boost::coroutines2::coroutine<impl::TaskContext*>::push_type& coro);

    void RegisterThread();

    static impl::CountedCoroutinePtr* GetCurrentTaskCoroutine() noexcept;

    void AccountStackUsage();
    std::uint16_t GetMaxStackUsagePct() const noexcept;
    bool IsActive() const noexcept;

    static bool DebugCanUseUserfaultfd();

private:
    class Impl;
    utils::FastPimpl<Impl, 1024, 8> impl_;
};

// It was discovered experimentally that registering more than ~4000 coroutines at the same time may crash or hang
// the service on Linux x86_64 after UFFDIO_REGISTER failure.
constexpr std::size_t kStackUsageMonitorLimit = 1000;

std::size_t GetCurrentTaskStackUsageBytes() noexcept;

const void* GetCoroCbPtr(const boost::coroutines2::coroutine<impl::TaskContext*>::push_type& coro) noexcept;

}  // namespace engine::coro

USERVER_NAMESPACE_END
