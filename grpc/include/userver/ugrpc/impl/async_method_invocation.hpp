#pragma once

#include <userver/compiler/impl/lifetime.hpp>
#include <userver/engine/awaitable.hpp>
#include <userver/engine/deadline.hpp>
#include <userver/engine/single_use_event.hpp>

#include <userver/ugrpc/impl/event_base.hpp>

USERVER_NAMESPACE_BEGIN

namespace ugrpc::impl {

class AsyncMethodInvocation final {
public:
    ~AsyncMethodInvocation();

    /// @brief For use from coroutines
    /// @return This object's `void* tag` for `grpc::CompletionQueue::Next`
    void* GetCompletionTag() noexcept;

    enum class WaitStatus {
        kOk,
        kError,
        kCancelled,
        // This constant means that engine::Deadline specified for wait operation
        // has expired. Note, that it is not related to the gRPC deadline
        kDeadline,
    };

    /// @brief For use from coroutines
    /// @return `bool ok` returned by `grpc::CompletionQueue::Next`
    [[nodiscard]] WaitStatus Wait() noexcept;

    /// @brief For use from coroutines
    /// @return `bool ok` returned by `grpc::CompletionQueue::Next` or
    ///          information regarding readiness
    [[nodiscard]] WaitStatus WaitUntil(engine::Deadline deadline) noexcept;

    /// @brief For use from coroutines
    /// @return `bool ok` returned by `grpc::CompletionQueue::Next`, ignoring task cancellations.
    [[nodiscard]] bool WaitNonCancellable() noexcept;

    /// @brief Checks if the asynchronous call has completed
    /// @return true if event returned from `grpc::CompletionQueue::Next`
    [[nodiscard]] bool IsReady() const noexcept;

    engine::AwaitableToken GetAwaitableToken() noexcept USERVER_IMPL_LIFETIME_BOUND;

private:
    class CompletionEvent final : public EventBase {
    public:
        explicit CompletionEvent(AsyncMethodInvocation& invocation) noexcept
            : invocation_{invocation}
        {}

        void Notify(bool ok) noexcept override { invocation_.OnComplete(ok); }

    private:
        AsyncMethodInvocation& invocation_;
    };

    void OnComplete(bool ok) noexcept;

    bool enqueued_{false};
    bool ok_{false};
    engine::SingleUseEvent event_;
    CompletionEvent completion_event_{*this};
};

}  // namespace ugrpc::impl

USERVER_NAMESPACE_END
