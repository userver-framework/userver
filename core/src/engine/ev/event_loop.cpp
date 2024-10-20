#include "event_loop.hpp"

#include <chrono>
#include <stdexcept>

#include <sys/param.h>
#include <sys/types.h>
#include <sys/wait.h>

#include <userver/compiler/demangle.hpp>
#include <userver/engine/task/cancel.hpp>
#include <userver/logging/log.hpp>
#include <userver/utils/assert.hpp>
#include <userver/utils/thread_name.hpp>

#include <utils/check_syscall.hpp>

#include "child_process_map.hpp"

USERVER_NAMESPACE_BEGIN

namespace engine::ev {
namespace {

std::atomic_flag& GetEvDefaultLoopFlag() noexcept {
    static std::atomic_flag ev_default_loop_flag ATOMIC_FLAG_INIT;
    return ev_default_loop_flag;
}

void AcquireEvDefaultLoop() {
    if (GetEvDefaultLoopFlag().test_and_set())
        throw std::runtime_error(
            "Trying to use more than one ev_default_loop, thread_name=" + utils::GetCurrentThreadName()
        );
    LOG_DEBUG() << "Acquire ev_default_loop for thread_name=" << utils::GetCurrentThreadName();
}

void ReleaseEvDefaultLoop() {
    LOG_DEBUG() << "Release ev_default_loop";
    GetEvDefaultLoopFlag().clear();
}

}  // namespace

EventLoop::EventLoop(EvLoopType ev_loop_mode) : ev_loop_mode_(ev_loop_mode) {
    if (ev_loop_mode_ == EvLoopType::kDefaultLoop) AcquireEvDefaultLoop();
    Start();
}

EventLoop::~EventLoop() {
    if (ev_loop_mode_ == EvLoopType::kDefaultLoop) {
        ev_child_stop(GetEvLoop(), &watch_child_);
        ReleaseEvDefaultLoop();
    } else {
        ev_loop_destroy(loop_);
    }
}

void EventLoop::RunOnce() noexcept {
    UASSERT(DebugIsSameOsThread());
    ev_run(loop_, EVRUN_ONCE);
}

void EventLoop::RunInEvLoopAsync(AsyncPayloadBase& payload) noexcept {
    UASSERT(DebugIsSameOsThread());
    payload.PerformAndRelease();
}

bool EventLoop::DebugIsSameOsThread() noexcept {
#ifndef NDEBUG
    if (os_thread_id_ != std::thread::id()) {
        return (os_thread_id_ == std::this_thread::get_id());
    }
    os_thread_id_ = std::this_thread::get_id();
#endif

    return true;
}

void EventLoop::Start() {
    loop_ = ((ev_loop_mode_ == EvLoopType::kDefaultLoop) ? ev_default_loop(EVFLAG_AUTO) : ev_loop_new(EVFLAG_AUTO));

    UASSERT(loop_);
#ifdef EV_HAS_IO_PESSIMISTIC_REMOVE
    ev_set_io_pessimistic_remove(loop_);
#endif

    if (ev_loop_mode_ == EvLoopType::kDefaultLoop) {
        // NOLINTNEXTLINE(cppcoreguidelines-pro-type-cstyle-cast)
        ev_child_init(&watch_child_, ChildWatcher, 0, 0);
        ev_child_start(loop_, &watch_child_);
    }
}

void EventLoop::ChildWatcher(struct ev_loop*, ev_child* w, int) noexcept {
    try {
        ChildWatcherImpl(w);
    } catch (const std::exception& ex) {
        LOG_ERROR() << "Exception in ChildWatcherImpl(): " << ex;
    }
}

void EventLoop::ChildWatcherImpl(ev_child* w) {
    auto* child_process_info = ChildProcessMapGetOptional(w->rpid);
    UASSERT_MSG(
        child_process_info,
        "Don't use 'system' to start subprocesses, use "
        "components::ProcessStarter instead"
    );
    if (!child_process_info) {
        LOG_ERROR() << "Got signal for thread with pid=" << w->rpid << ", status=" << w->rstatus
                    << ", but thread with this pid was not found in child_process_map. "
                       "Don't use 'system' to start subprocesses, use "
                       "components::ProcessStarter instead";
        return;
    }

    auto process_status = subprocess::ChildProcessStatus{
        w->rstatus,
        std::chrono::duration_cast<std::chrono::milliseconds>(
            std::chrono::steady_clock::now() - child_process_info->start_time
        )};
    if (process_status.IsExited() || process_status.IsSignaled()) {
        LOG_INFO() << "Child process with pid=" << w->rpid << " was "
                   << (process_status.IsExited() ? "exited normally" : "terminated by a signal");
        child_process_info->status_promise.set_value(std::move(process_status));
        ChildProcessMapErase(w->rpid);
    } else {
        if (WIFSTOPPED(w->rstatus)) {
            LOG_WARNING() << "Child process with pid=" << w->rpid
                          << " was stopped with signal=" << WSTOPSIG(w->rstatus);
        } else {
            const bool continued = WIFCONTINUED(w->rstatus);
            if (continued) {
                LOG_WARNING() << "Child process with pid=" << w->rpid << " was resumed";
            } else {
                LOG_WARNING() << "Child process with pid=" << w->rpid
                              << " was notified in ChildWatcher with unknown reason (w->rstatus=" << w->rstatus << ')';
            }
        }
    }
}

}  // namespace engine::ev

USERVER_NAMESPACE_END
