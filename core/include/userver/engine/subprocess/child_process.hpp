#pragma once

/// @file
/// @brief @copybrief engine::subprocess::ChildProcess

#include <chrono>
#include <memory>

#include <userver/compiler/select.hpp>
#include <userver/engine/deadline.hpp>
#include <userver/engine/subprocess/child_process_status.hpp>
#include <userver/utils/fast_pimpl.hpp>

USERVER_NAMESPACE_BEGIN

namespace engine::subprocess {

class ChildProcessImpl;

/// @brief Class that allows controlling child process; usually retrieved from engine::subprocess::ProcessStarter
class ChildProcess final {
public:
    /// @cond
    explicit ChildProcess(ChildProcessImpl&& impl) noexcept;
    /// @endcond

    ChildProcess(ChildProcess&&) noexcept;
    ChildProcess& operator=(ChildProcess&&) noexcept;

    /// @brief Does not terminate the child process (just detaches from it).
    ~ChildProcess();

    /// @brief Returns pid of the child process.
    int GetPid() const;

    /// @brief Wait for the child process to terminate.
    /// Ignores cancellations of the current task.
    void Wait();

    /// @brief Wait for the child process to terminate.
    /// @returns false if this did not happen for the specified timeout duration or a task cancellation happened; true
    /// otherwise.
    template <typename Rep, typename Period>
    [[nodiscard]] bool WaitFor(std::chrono::duration<Rep, Period> duration) {
        return WaitUntil(Deadline::FromDuration(duration));
    }

    /// @brief Wait for the child process to terminate.
    /// @returns false if this did not happen until the specified time point has been reached or a task cancellation
    // happened; true otherwise.
    template <typename Clock, typename Duration>
    [[nodiscard]] bool WaitUntil(std::chrono::time_point<Clock, Duration> until) {
        return WaitUntil(Deadline::FromTimePoint(until));
    }

    /// Wait for the child process to terminate.
    /// Returns if this did not happen until the specified Deadline has been
    /// reached.
    [[nodiscard]] bool WaitUntil(Deadline deadline);

    /// @brief Wait for the child process to terminate, ignoring cancellations.
    /// @returns ChildProcessStatus of finished subprocess
    [[nodiscard]] ChildProcessStatus Get();

    /// @brief Send a signal to the child process.
    void SendSignal(int signum);

private:
    static constexpr std::size_t kImplSize = compiler::SelectSize().For64Bit(24).For32Bit(12);
    static constexpr std::size_t kImplAlignment = alignof(void*);
    utils::FastPimpl<ChildProcessImpl, kImplSize, kImplAlignment> impl_;
};

}  // namespace engine::subprocess

USERVER_NAMESPACE_END
