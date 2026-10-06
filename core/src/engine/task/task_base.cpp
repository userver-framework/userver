#include <userver/engine/task/task_base.hpp>

#include <atomic>

#include <engine/task/task_base_impl.hpp>
#include <engine/task/task_context.hpp>
#include <engine/task/task_processor.hpp>
#include <engine/task/task_processor_pools.hpp>
#include <userver/engine/async.hpp>
#include <userver/engine/impl/awaiter.hpp>
#include <userver/engine/impl/epoch.hpp>
#include <userver/engine/impl/task_context_holder.hpp>
#include <userver/engine/task/cancel.hpp>
#include <userver/utils/assert.hpp>

USERVER_NAMESPACE_BEGIN

namespace engine {

namespace {

class BlockingWaitAwaiter final : public impl::PolymorphicAwaiter {
public:
    explicit BlockingWaitAwaiter(impl::TaskContext& context) noexcept
        : context_(context)
    {}

    void NotifyAndDispose(std::uintptr_t context) noexcept override {
        UASSERT(context == 0);
        auto& finish_flag = context_.BlockingWaitFinishFlag();
        finish_flag.store(true, std::memory_order_release);
        finish_flag.notify_all();
    }

    void DisposeWithoutNotification() noexcept override {
        utils::AbortWithStacktrace("BlockingWaitAwaiter should never be removed without notification");
    }

private:
    impl::TaskContext& context_;
};

}  // namespace

static_assert(
    !std::is_destructible_v<TaskBase>,
    "Destructor of TaskBase must remain protected to forbid slicing of derived "
    "types to TaskBase and force implementation in derived classes."
);

static_assert(!std::is_polymorphic_v<TaskBase>, "Slicing is used by derived types, virtual functions would not work.");

TaskBase::TaskBase(impl::TaskContextHolder&& context)
    : pimpl_(Impl{std::move(context).Extract()})
{
    impl::TaskContext::Wakeup(
        boost::intrusive_ptr<impl::TaskContext>{pimpl_->context},
        impl::TaskContext::WakeupSource::kBootstrap,
        impl::Epoch{0}
    );
}

bool TaskBase::IsValid() const { return !!pimpl_->context; }

Task::State TaskBase::GetState() const { return pimpl_->context ? pimpl_->context->GetState() : State::kInvalid; }

std::string_view TaskBase::GetStateName(State state) {
    switch (state) {
        case State::kInvalid:
            return "kInvalid";
        case State::kNew:
            return "kNew";
        case State::kQueued:
            return "kQueued";
        case State::kRunning:
            return "kRunning";
        case State::kSuspended:
            return "kSuspended";
        case State::kCancelled:
            return "kCancelled";
        case State::kCompleted:
            return "kCompleted";
    }

    UINVARIANT(false, "Unexpected Task state");
}

bool TaskBase::IsFinished() const { return pimpl_->context && pimpl_->context->IsFinished(); }

void TaskBase::Wait() const noexcept(false) { WaitUntil(Deadline{}); }

void TaskBase::WaitUntil(Deadline deadline) const {
    const auto status = WaitNothrowUntil(deadline);
    if (status == FutureStatus::kCancelled) {
        throw WaitInterruptedException(current_task::CancellationReason());
    }
}

bool TaskBase::WaitNothrow() const noexcept { return WaitNothrowUntil(Deadline{}) == FutureStatus::kReady; }

FutureStatus TaskBase::WaitNothrowUntil(Deadline deadline) const noexcept { return GetContext().WaitUntil(deadline); }

void TaskBase::RequestCancel() { GetContext().RequestCancel(TaskCancellationReason::kUserRequest); }

void TaskBase::SyncCancel() noexcept { Terminate(TaskCancellationReason::kUserRequest); }

TaskCancellationReason TaskBase::CancellationReason() const { return GetContext().CancellationReason(); }

void TaskBase::BlockingWait() const noexcept {
    UASSERT(pimpl_->context);
    UASSERT(!current_task::IsTaskProcessorThread());

    auto& context = *pimpl_->context;
    auto& finish_flag = context.BlockingWaitFinishFlag();

    // Do not store the flag in BlockingWaitAwaiter: that object is a local of BlockingWait and is destroyed as soon as
    // wait() returns. wait() may return after seeing the store while the other thread is still inside notify_all().
    // Destroying the atomic at that point causes UB (P2616 / [basic.life]). TaskContext outlives BlockingWait, so the
    // flag stays alive until notify_all() finishes.
    BlockingWaitAwaiter awaiter{context};
    impl::AwaiterPtr awaiter_ptr{&awaiter};
    context.TryAppendAwaiter(awaiter_ptr, 0);
    if (awaiter_ptr != nullptr) {
        impl::NotifyAndDispose(std::move(awaiter_ptr), 0);
    }
    finish_flag.wait(false);

    UASSERT(context.IsFinished());
}

void TaskBase::Invalidate() noexcept {
    Terminate(TaskCancellationReason::kAbandoned);
    pimpl_->context.reset();
}

TaskBase::TaskBase() = default;
TaskBase::~TaskBase() = default;

TaskBase::TaskBase(TaskBase&&) noexcept = default;
TaskBase& TaskBase::operator=(TaskBase&&) noexcept = default;

// NOLINTNEXTLINE(hicpp-use-equals-default,modernize-use-equals-default)
TaskBase::TaskBase(const TaskBase& other) noexcept : pimpl_(other.pimpl_) {}

// NOLINTNEXTLINE(hicpp-use-equals-default,modernize-use-equals-default,cert-oop54-cpp)
TaskBase& TaskBase::operator=(const TaskBase& other) noexcept {
    pimpl_->context = other.pimpl_->context;
    return *this;
}

impl::TaskContext& TaskBase::GetContext() const noexcept {
    UASSERT(pimpl_->context);
    return *pimpl_->context;
}

bool TaskBase::HasSameContext(const TaskBase& other) const noexcept { return pimpl_->context == other.pimpl_->context; }

utils::impl::WrappedCallBase& TaskBase::GetPayload() const noexcept { return GetContext().GetPayload(); }

void TaskBase::Terminate(TaskCancellationReason reason) noexcept {
    if (!pimpl_->context) {
        return;
    }

    if (!IsFinished()) {
        // We are not providing an implicit sync from outside
        // because it's really easy to get a deadlock this way
        // e.g. between global event thread pool and task processor
        pimpl_->context->RequestCancel(reason);

        const TaskCancellationBlocker cancel_blocker;
        Wait();
    }

    if (reason == TaskCancellationReason::kAbandoned) {
        pimpl_->context->ResetPayload();
    }
}

namespace current_task {

bool IsTaskProcessorThread() noexcept { return GetCurrentTaskContextUnchecked() != nullptr; }

TaskProcessor& GetTaskProcessor() { return GetCurrentTaskContext().GetTaskProcessor(); }

std::size_t GetWorkerCount() { return GetTaskProcessor().GetWorkerCount(); }

TaskProcessor& GetBlockingTaskProcessor() { return GetTaskProcessor().GetBlockingTaskProcessor(); }

std::size_t GetStackSize() { return GetTaskProcessor().GetTaskProcessorPools()->GetCoroPool().GetStackSize(); }

ev::ThreadControl& GetEventThread() { return GetTaskProcessor().EventThreadPool().NextThread(); }

namespace impl {

void* GetRawCurrentTaskContext() noexcept { return current_task::GetCurrentTaskContextUnchecked(); }

bool IsCritical() { return GetCurrentTaskContext().WasStartedAsCritical(); }

Deadline GetDeadline() noexcept { return GetCurrentTaskContext().GetCancelDeadline(); }

}  // namespace impl

}  // namespace current_task

namespace impl {

std::uint64_t GetCreatedTaskCount(TaskProcessor& task_processor) {
    return task_processor.GetTaskCounter().GetCreatedTasks().value;
}

}  // namespace impl

}  // namespace engine

USERVER_NAMESPACE_END
