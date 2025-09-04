#include <engine/ev/watcher/async_watcher.hpp>

#include <engine/ev/thread.hpp>
#include <userver/engine/single_consumer_event.hpp>
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

USERVER_NAMESPACE_END
