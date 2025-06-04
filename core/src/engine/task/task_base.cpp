#include <userver/engine/task/task_base.hpp>

#include <future>

#include <engine/impl/generic_wait_list.hpp>
#include <engine/task/task_base_impl.hpp>
#include <engine/task/task_context.hpp>
#include <engine/task/task_processor.hpp>
#include <engine/task/task_processor_pools.hpp>
#include <userver/engine/async.hpp>
#include <userver/engine/impl/task_context_holder.hpp>
#include <userver/engine/task/cancel.hpp>
#include <userver/utils/assert.hpp>

USERVER_NAMESPACE_BEGIN

namespace engine {

static_assert(
    !std::is_destructible_v<TaskBase>,
    "Destructor of TaskBase must remain protected to forbid slicing of derived "
    "types to TaskBase and force implementation in derived classes."
);

static_assert(!std::is_polymorphic_v<TaskBase>, "Slicing is used by derived types, virtual functions would not work.");

TaskBase::TaskBase(impl::TaskContextHolder&& context) : pimpl_(Impl{std::move(context).Extract()}) {
    pimpl_->context->Wakeup(impl::TaskContext::WakeupSource::kBootstrap, impl::SleepState::Epoch{0});
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

void TaskBase::BlockingWait() const {
    UASSERT(pimpl_->context);
    UASSERT(!current_task::IsTaskProcessorThread());

    auto& context = *pimpl_->context;
    if (context.IsFinished()) return;

    std::packaged_task<void()> task([&context] {
        const TaskCancellationBlocker block_cancels;
        const auto status = context.WaitUntil(Deadline{});
        UASSERT(status == FutureStatus::kReady);
    });
    auto future = task.get_future();

    engine::DetachUnscopedUnsafe(engine::CriticalAsyncNoSpan(context.GetTaskProcessor(), std::move(task)));
    future.wait();
    future.get();
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

TaskProcessor& GetBlockingTaskProcessor() { return GetTaskProcessor().GetBlockingTaskProcessor(); }

std::size_t GetStackSize() { return GetTaskProcessor().GetTaskProcessorPools()->GetCoroPool().GetStackSize(); }

ev::ThreadControl& GetEventThread() { return GetTaskProcessor().EventThreadPool().NextThread(); }

namespace impl {

void* GetRawCurrentTaskContext() noexcept { return current_task::GetCurrentTaskContextUnchecked(); }

}  // namespace impl

}  // namespace current_task

namespace impl {

std::uint64_t GetCreatedTaskCount(TaskProcessor& task_processor) {
    return task_processor.GetTaskCounter().GetCreatedTasks().value;
}

}  // namespace impl

}  // namespace engine

USERVER_NAMESPACE_END
