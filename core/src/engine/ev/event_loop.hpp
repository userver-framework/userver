#pragma once

#include <thread>

#include <ev.h>

#include <engine/ev/async_payload_base.hpp>

USERVER_NAMESPACE_BEGIN

namespace engine::ev {

class EventLoop final {
public:
    enum class EvLoopType : bool {
        kNewLoop,
        kDefaultLoop,
    };

    explicit EventLoop(EvLoopType ev_loop_mode);

    ~EventLoop();

    struct ev_loop* GetEvLoop() const noexcept { return loop_; }

    void RunOnce() noexcept;

    // Callbacks passed to RunInEvLoopAsync() are serialized.
    // All callbacks are guaranteed to execute.
    void RunInEvLoopAsync(AsyncPayloadBase& payload) noexcept;

    bool DebugIsSameOsThread() noexcept;

private:
    void AssertSameOsThread() noexcept;

    void Start();

    static void ChildWatcher(struct ev_loop*, ev_child* w, int) noexcept;
    static void ChildWatcherImpl(ev_child* w);

    struct ev_loop* loop_{nullptr};
    ev_child watch_child_{};

    const EvLoopType ev_loop_mode_;

#ifndef NDEBUG
    std::thread::id os_thread_id_{};
#endif
};

}  // namespace engine::ev

USERVER_NAMESPACE_END
