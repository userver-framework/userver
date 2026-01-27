#pragma once

#include <mutex>

#include <engine/task/task_counter.hpp>

#include <userver/concurrent/impl/interference_shield.hpp>
#include <userver/engine/task/task_processor_fwd.hpp>
#include <userver/utils/assert.hpp>
#include <userver/utils/fixed_array.hpp>

USERVER_NAMESPACE_BEGIN

namespace engine {

class TaskProcessorMutexSet final {
public:
    explicit TaskProcessorMutexSet(TaskProcessor& tp, std::size_t worker_count);

    std::lock_guard<std::mutex> ReadLockFromCoroutine() noexcept;

    using Lock = utils::FixedArray<std::lock_guard<std::mutex>>;

    Lock WriteLock() noexcept;

    bool MayReadLock() const noexcept;

private:
    TaskProcessor& tp_;
    utils::FixedArray<concurrent::impl::InterferenceShield<std::mutex>> mutexes_;
};

}  // namespace engine

USERVER_NAMESPACE_END
