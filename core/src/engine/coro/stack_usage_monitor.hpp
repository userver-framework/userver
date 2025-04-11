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

std::size_t GetCurrentTaskStackUsageBytes() noexcept;

}  // namespace engine::coro

USERVER_NAMESPACE_END
