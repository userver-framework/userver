#include <ugrpc/impl/event_engine/engine.hpp>

#if defined(ARCADIA_ROOT) && defined(__linux__)

#include <grpc/event_engine/event_engine.h>
#include <y_absl/functional/any_invocable.h>
#include <y_absl/status/status.h>
#include <y_absl/status/statusor.h>

#include <atomic>
#include <exception>
#include <memory>
#include <new>
#include <optional>
#include <stdexcept>
#include <utility>

#include <ev.h>

#include <userver/engine/single_use_event.hpp>
#include <userver/engine/task/cancel.hpp>
#include <userver/engine/task/current_task.hpp>
#include <userver/utils/assert.hpp>

#include <engine/ev/thread_control.hpp>
#include <engine/ev/thread_pool.hpp>
#include <engine/task/task_processor.hpp>
#include <ugrpc/impl/event_engine/ev_epoll1_linux.hpp>
#include <ugrpc/impl/event_engine/executor.hpp>
#include <ugrpc/impl/event_engine/listener.hpp>
#include <ugrpc/impl/event_engine/timer_manager.hpp>

USERVER_NAMESPACE_BEGIN

namespace ugrpc::impl::event_engine {

namespace {

engine::TaskProcessor& CheckCoroutineContext(engine::TaskProcessor& task_processor) {
    UINVARIANT(
        engine::current_task::IsTaskProcessorThread(),
        "Create PosixEventEngineOwner in a coroutine so initialization failures can drain safely"
    );
    return task_processor;
}

void CheckShutdownContext() {
    UINVARIANT(
        engine::current_task::IsTaskProcessorThread(),
        "Shut down the PosixEventEngine from a coroutine before stopping its TaskProcessor"
    );
    UINVARIANT(
        !Executor::IsInCallback(),
        "Run the PosixEventEngine shutdown owner outside gRPC callbacks and their capture destructors"
    );
}

class Epoll1Watcher final {
public:
    Epoll1Watcher(engine::ev::ThreadControl control, Epoll1Poller& poller) noexcept : control_(control), poller_(poller) {
        ev_io_init(&io_, &Epoll1Watcher::OnEpollReadable, poller_.EpollFd(), EV_READ);
        io_.data = this;
        control_.RunInEvLoopSync([this]() noexcept { control_.Start(io_); });
    }

    ~Epoll1Watcher() {
        constexpr double kImmediateDelay = 0.0;
        constexpr double kNoRepeat = 0.0;
        engine::SingleUseEvent pending_updates_applied;
        ev_timer after_pending_updates{};
        ev_timer_init(&after_pending_updates, &OnPendingUpdatesApplied, kImmediateDelay, kNoRepeat);
        after_pending_updates.data = &pending_updates_applied;
        control_.RunInEvLoopSync([this, &after_pending_updates]() noexcept {
            control_.Stop(io_);
            control_.Start(after_pending_updates);
        });
        pending_updates_applied.WaitNonCancellable();
    }

    Epoll1Watcher(const Epoll1Watcher&) = delete;
    Epoll1Watcher& operator=(const Epoll1Watcher&) = delete;

private:
    static void OnPendingUpdatesApplied(struct ev_loop*, ev_timer* timer, int) noexcept {
        static_cast<engine::SingleUseEvent*>(timer->data)->Send();
    }

    static void OnEpollReadable(struct ev_loop*, ev_io* io, int) noexcept {
        static_cast<Epoll1Watcher*>(io->data)->poller_.Work();
    }

    engine::ev::ThreadControl control_;
    Epoll1Poller& poller_;
    ev_io io_{};
};

}  // namespace

struct PosixEventEngine::Impl {
    enum class Phase { kRunning, kShuttingDown, kStopped };

    explicit Impl(engine::TaskProcessor& processor)
        : executor(processor),
          timer_manager(processor.EventThreadPool(), executor)
    {
        watcher.emplace(processor.EventThreadPool().NextThread(), poller);
    }

    std::atomic<Phase> phase{Phase::kRunning};
    Epoll1Poller poller;
    Executor executor;
    TimerManager timer_manager;
    std::optional<Epoll1Watcher> watcher;
};

PosixEventEngine::PosixEventEngine(engine::TaskProcessor& task_processor)
    : impl_(CheckCoroutineContext(task_processor))
{}
PosixEventEngine::~PosixEventEngine() {
    UINVARIANT(IsStopped(), "Keep PosixEventEngineOwner alive and call Shutdown before releasing the PosixEventEngine");
}
engine::TaskProcessor& PosixEventEngine::GetTaskProcessor() const noexcept {
    return impl_->executor.GetTaskProcessor();
}
bool PosixEventEngine::IsStopping() const noexcept { return impl_->phase.load() != Impl::Phase::kRunning; }
bool PosixEventEngine::IsStopped() const noexcept { return impl_->phase.load() == Impl::Phase::kStopped; }

void PosixEventEngine::Shutdown() noexcept {
    if (IsStopped()) {
        return;
    }
    CheckShutdownContext();
    const engine::TaskCancellationBlocker cancellation_blocker;
    auto expected = Impl::Phase::kRunning;
    UINVARIANT(
        impl_->phase.compare_exchange_strong(expected, Impl::Phase::kShuttingDown),
        "Only one PosixEventEngine shutdown owner may stop callbacks at a time"
    );
    impl_->watcher.reset();
    impl_->timer_manager.Shutdown();
    impl_->executor.Quiesce();
    impl_->poller.Shutdown();
    impl_->phase.store(Impl::Phase::kStopped);
}

void PosixEventEngine::Run(Closure* closure) { impl_->executor.Run(closure); }
void PosixEventEngine::Run(AnyInvocable closure) { impl_->executor.Run(std::move(closure)); }
void PosixEventEngine::Post(AnyInvocable closure) noexcept { impl_->executor.Post(std::move(closure)); }
PosixEventEngine::TaskHandle PosixEventEngine::RunAfter(Duration when, Closure* closure) {
    UASSERT(closure);
    return impl_->timer_manager.TimerInit(when, [closure] { closure->Run(); });
}
PosixEventEngine::TaskHandle PosixEventEngine::RunAfter(Duration when, AnyInvocable closure) {
    return impl_->timer_manager.TimerInit(when, std::move(closure));
}
bool PosixEventEngine::Cancel(TaskHandle handle) { return impl_->timer_manager.TimerCancel(handle); }
bool PosixEventEngine::IsWorkerThread() { utils::AbortWithStacktrace("unimplemented"); }

y_absl::StatusOr<std::unique_ptr<grpc_event_engine::experimental::PosixListenerWithFdSupport>>
PosixEventEngine::CreatePosixListener(
    PosixAcceptCallback on_accept,
    y_absl::AnyInvocable<void(y_absl::Status)> on_shutdown,
    const grpc_event_engine::experimental::EndpointConfig& config,
    std::unique_ptr<grpc_event_engine::experimental::MemoryAllocatorFactory> memory_allocator_factory
) {
    if (IsStopping()) {
        return y_absl::FailedPreconditionError(
            "The PosixEventEngine is stopping; create listeners before owner shutdown"
        );
    }
    if (!on_accept || !on_shutdown || !memory_allocator_factory) {
        return y_absl::InvalidArgumentError("Provide on_accept, on_shutdown and a memory allocator factory");
    }
    try {
        std::unique_ptr<grpc_event_engine::experimental::PosixListenerWithFdSupport> listener = std::make_unique<
            event_engine::PosixEngineListener>(
            std::move(on_accept),
            std::move(on_shutdown),
            config,
            std::move(memory_allocator_factory),
            impl_->poller,
            std::static_pointer_cast<PosixEventEngine>(shared_from_this())
        );
        return listener;
    } catch (const std::bad_alloc& error) {
        return y_absl::ResourceExhaustedError(error.what());
    } catch (const std::exception& error) {
        return y_absl::InvalidArgumentError(error.what());
    }
}
y_absl::StatusOr<std::unique_ptr<grpc_event_engine::experimental::EventEngine::Listener>> PosixEventEngine::
    CreateListener(grpc_event_engine::experimental::EventEngine::Listener::AcceptCallback, y_absl::AnyInvocable<void(y_absl::Status)>, const grpc_event_engine::experimental::EndpointConfig&, std::unique_ptr<grpc_event_engine::experimental::MemoryAllocatorFactory>) {
    utils::AbortWithStacktrace("unimplemented");
}
std::unique_ptr<grpc_event_engine::experimental::PosixEndpointWithFdSupport>
PosixEventEngine::CreatePosixEndpointFromFd(
    int,
    const grpc_event_engine::experimental::EndpointConfig&,
    grpc_event_engine::experimental::MemoryAllocator
) {
    utils::AbortWithStacktrace("unimplemented");
}
PosixEventEngine::ConnectionHandle PosixEventEngine::Connect(
    OnConnectCallback,
    const ResolvedAddress&,
    const grpc_event_engine::experimental::EndpointConfig&,
    grpc_event_engine::experimental::MemoryAllocator,
    Duration
) {
    utils::AbortWithStacktrace(
        "Connect is not implemented; disable event_engine_client for the userver server PosixEventEngine"
    );
}
bool PosixEventEngine::CancelConnect(ConnectionHandle) {
    utils::AbortWithStacktrace(
        "CancelConnect is not implemented; disable event_engine_client for the userver server PosixEventEngine"
    );
}
y_absl::StatusOr<std::unique_ptr<grpc_event_engine::experimental::EventEngine::DNSResolver>>
PosixEventEngine::GetDNSResolver(const DNSResolver::ResolverOptions&) {
    utils::AbortWithStacktrace("GetDNSResolver is not implemented; disable event_engine_dns");
}

}  // namespace ugrpc::impl::event_engine

USERVER_NAMESPACE_END

#endif
