#include <engine/ev/watcher/async_watcher.hpp>

#include <future>
#include <memory>

#include <engine/ev/thread.hpp>
#include <userver/engine/async.hpp>
#include <userver/engine/single_consumer_event.hpp>
#include <userver/engine/task/current_task.hpp>
#include <userver/utest/utest.hpp>

USERVER_NAMESPACE_BEGIN

constexpr std::chrono::milliseconds kContextSwitchTimeout{10};

UTEST(AsyncWatcher, Basic) {
    engine::ev::Thread thread{"test_thread"};
    engine::ev::ThreadControl thread_control{thread};

    engine::SingleConsumerEvent event;

    engine::ev::AsyncWatcher watcher(thread_control, [&event]() { event.Send(); });
    EXPECT_FALSE(event.WaitForEventFor(kContextSwitchTimeout));

    watcher.Start();
    EXPECT_FALSE(event.WaitForEventFor(kContextSwitchTimeout));

    watcher.Send();
    EXPECT_TRUE(event.WaitForEvent());

    EXPECT_FALSE(event.IsReady());
    watcher.Send();
    EXPECT_FALSE(event.WaitForEventFor(kContextSwitchTimeout));

    watcher.Start();
    EXPECT_FALSE(event.WaitForEventFor(kContextSwitchTimeout));

    watcher.Send();
    EXPECT_TRUE(event.WaitForEvent());
}

UTEST(AsyncWatcher, Stop) {
    engine::ev::Thread thread{"test_thread"};
    engine::ev::ThreadControl thread_control{thread};

    engine::SingleConsumerEvent event;

    engine::ev::AsyncWatcher watcher(thread_control, [&event]() { event.Send(); });
    watcher.Start();

    // Redis driver relies on the following behavior on driver stop
    watcher.Stop();
    EXPECT_FALSE(event.IsReady());
    watcher.Send();
    EXPECT_FALSE(event.WaitForEventFor(kContextSwitchTimeout));
}

UTEST(AsyncWatcher, KeepsCallbackAliveUntilPendingStartIsDrained) {
    auto& control = engine::current_task::GetEventThread();
    std::promise<void> release_loop;
    engine::SingleConsumerEvent destroying;

    auto token = std::make_shared<int>(0);
    const std::weak_ptr<int> lifetime = token;
    auto watcher = std::make_unique<engine::ev::AsyncWatcher>(control, [token = std::move(token)] {});
    auto observer = engine::AsyncNoTracing([&] {
        EXPECT_TRUE(destroying.WaitForEvent());
        // UTEST has one worker. Destruction has suspended waiting for the ev thread.
        const bool alive = !lifetime.expired();
        release_loop.set_value();
        return alive;
    });

    control.RunInEvLoopAsync([gate = release_loop.get_future()] { gate.wait(); });
    watcher->Start();
    destroying.Send();
    watcher.reset();

    EXPECT_TRUE(observer.Get());
    EXPECT_TRUE(lifetime.expired());
}

USERVER_NAMESPACE_END
