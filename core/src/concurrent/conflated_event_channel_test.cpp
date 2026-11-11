#include <userver/concurrent/conflated_event_channel.hpp>

#include <utility>

#include <userver/engine/mutex.hpp>
#include <userver/engine/semaphore.hpp>
#include <userver/utils/resource_scopes.hpp>

#include <userver/utest/utest.hpp>

USERVER_NAMESPACE_BEGIN

namespace {

enum class ListenMode { kSubscribe, kUpdateAndSubscribe };

struct Subscriber final {
    Subscriber(
        utils::ResourceScopeStorage& scopes,
        concurrent::ConflatedEventChannel& channel,
        ListenMode mode = ListenMode::kSubscribe
    )
        : sem(1)
    {
        sem.lock_shared();  // To set semaphore 0 at start
        if (mode == ListenMode::kUpdateAndSubscribe) {
            channel.UpdateAndListen(scopes, this, "sub", &Subscriber::OnEvent);
        } else {
            channel.AddListener(scopes, this, "sub", &Subscriber::OnEvent);
        }
    }

    void OnEvent() {
        sem.unlock_shared();
        const std::lock_guard<engine::Mutex> lock(mutex);
        times_called++;
    }

    ~Subscriber() {
        sem.unlock_shared();  // Return semaphore to 1
    }

    int times_called = 0;
    engine::Mutex mutex;
    engine::Semaphore sem;
};

}  // namespace

UTEST(ConflatedEventChannel, Publish) {
    concurrent::AsyncEventChannel channel("channel");
    concurrent::ConflatedEventChannel conflated_channel("conflated_channel");
    conflated_channel.AddChannel(channel);
    utils::WithResourceScopes<Subscriber> subscriber(std::in_place, conflated_channel);

    EXPECT_EQ(subscriber->times_called, 0);

    channel.SendEvent();

    subscriber->sem.lock_shared();
    EXPECT_EQ(subscriber->times_called, 1);
}

UTEST(ConflatedEventChannel, PublishMany) {
    concurrent::AsyncEventChannel channel("channel");
    concurrent::ConflatedEventChannel conflated_channel("conflated_channel");
    conflated_channel.AddChannel(channel);

    utils::WithResourceScopes<Subscriber> subscriber(std::in_place, conflated_channel);

    EXPECT_EQ(subscriber->times_called, 0);

    {
        const std::lock_guard<engine::Mutex> lock(subscriber->mutex);

        // E1 calls 'OnEvent' asynchronously, which increments 'sem' and starts
        // waiting for 'mutex'. If 'SendEvent' was synchronous, a deadlock would
        // occur here.
        channel.SendEvent();

        // Wait for E1 to be received. When called rapidly, E2, E3 and E4 could
        // all be conflated with E1, and we don't want it in this test.
        subscriber->sem.lock_shared();
        channel.SendEvent();  // E2
        channel.SendEvent();  // E3 pushes out E2
        channel.SendEvent();  // E4 pushes out E3

        // 'mutex' is unlocked, E1 completes, then E4 is processed
    }

    subscriber->sem.lock_shared();  // unlocked by E4
    EXPECT_EQ(subscriber->times_called, 2);

    // Same as above, but events are sent manually
    {
        const std::lock_guard<engine::Mutex> lock(subscriber->mutex);
        conflated_channel.SendEvent();
        subscriber->sem.lock_shared();
        conflated_channel.SendEvent();
        conflated_channel.SendEvent();
        conflated_channel.SendEvent();
    }
    subscriber->sem.lock_shared();
    EXPECT_EQ(subscriber->times_called, 4);
}

UTEST(ConflatedEventChannel, PublishFromManyChannels) {
    concurrent::AsyncEventChannel channel1("channel1");
    concurrent::AsyncEventChannel channel2("channel2");
    concurrent::ConflatedEventChannel conflated_channel("conflated_channel");
    conflated_channel.AddChannel(channel1);
    conflated_channel.AddChannel(channel2);

    utils::WithResourceScopes<Subscriber> subscriber(std::in_place, conflated_channel);

    EXPECT_EQ(subscriber->times_called, 0);

    channel1.SendEvent();
    subscriber->sem.lock_shared();
    EXPECT_EQ(subscriber->times_called, 1);

    channel2.SendEvent();
    subscriber->sem.lock_shared();
    EXPECT_EQ(subscriber->times_called, 2);

    {
        const std::lock_guard<engine::Mutex> lock(subscriber->mutex);
        channel1.SendEvent();
        subscriber->sem.lock_shared();
        channel2.SendEvent();
        channel1.SendEvent();
        channel2.SendEvent();
    }
    subscriber->sem.lock_shared();
    EXPECT_EQ(subscriber->times_called, 4);
}

UTEST(ConflatedEventChannel, PublishForManyListeners) {
    concurrent::AsyncEventChannel channel("channel");
    concurrent::ConflatedEventChannel conflated_channel("conflated_channel");
    conflated_channel.AddChannel(channel);

    utils::WithResourceScopes<Subscriber> subscriber1(std::in_place, conflated_channel);
    utils::WithResourceScopes<Subscriber> subscriber2(std::in_place, conflated_channel);

    EXPECT_EQ(subscriber1->times_called, 0);
    EXPECT_EQ(subscriber2->times_called, 0);

    channel.SendEvent();

    subscriber1->sem.lock_shared();
    subscriber2->sem.lock_shared();
    EXPECT_EQ(subscriber1->times_called, 1);
    EXPECT_EQ(subscriber2->times_called, 1);
}

UTEST(ConflatedEventChannel, UpdateAndListen) {
    concurrent::AsyncEventChannel channel("channel");
    concurrent::ConflatedEventChannel conflated_channel("conflated_channel");
    conflated_channel.AddChannel(channel);

    utils::WithResourceScopes<Subscriber> subscriber(std::in_place, conflated_channel, ListenMode::kUpdateAndSubscribe);
    EXPECT_EQ(subscriber->times_called, 1);

    channel.SendEvent();
    subscriber->sem.lock_shared();
    subscriber->sem.lock_shared();
    EXPECT_EQ(subscriber->times_called, 2);
}

UTEST(ConflatedEventChannel, ForwardingDataLivesUntilUnsubscribe) {
    int removals = 0;
    concurrent::AsyncEventChannel<> source("source", [&removals](const auto& callback) {
        ++removals;
        callback();
    });
    {
        concurrent::ConflatedEventChannel channel("conflated");
        concurrent::AsyncEventSource<>& event_source = source;
        channel.AddChannel(event_source);
    }
    EXPECT_EQ(removals, concurrent::impl::kCheckSubscriptionUB ? 1 : 0);
    source.SendEvent();
}

USERVER_NAMESPACE_END
