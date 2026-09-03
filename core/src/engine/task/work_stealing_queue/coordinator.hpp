#pragma once

#include <atomic>
#include <cstddef>
#include <cstdint>
#include <mutex>

#include <boost/intrusive/list.hpp>

#include <engine/task/work_stealing_queue/worker.hpp>

USERVER_NAMESPACE_BEGIN

namespace engine::fast {

class Coordinator final {
public:
    explicit Coordinator(std::size_t threads) noexcept;

    void NotifyNewTask() noexcept;

    void StepDownAsActiveWorker(Worker& worker) noexcept;

    void BecomeActiveWorker(Worker& worker) noexcept;

    void WakeWorker() noexcept;

    bool StartSpinning() noexcept;

    bool StopSpinning() noexcept;

    void RequestStop() noexcept;

    bool IsStopRequested() const noexcept;

private:
    struct StateView final {
        std::uint32_t idle;
        std::uint32_t spinning;
    };

    static StateView AsStateView(std::uint64_t state) noexcept;

    bool ShouldWakeWorker() const noexcept;

    void WakeAll() noexcept;

    void IncrIdleCount() noexcept;
    void DecrIdleCount() noexcept;

    using IdleList = boost::intrusive::list<
        Worker,
        boost::intrusive::base_hook<WorkerIdleHook>,
        boost::intrusive::constant_time_size<false>>;

    const std::size_t threads_;

    std::atomic<std::uint64_t> state_{0};
    std::mutex mutex_;
    IdleList idle_workers_;
    std::atomic<bool> stop_requested_{false};
};

}  // namespace engine::fast

USERVER_NAMESPACE_END
