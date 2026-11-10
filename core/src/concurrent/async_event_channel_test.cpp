#include <userver/utest/utest.hpp>

#include <atomic>
#include <optional>
#include <stdexcept>
#include <string>
#include <utility>
#include <vector>

#include <gmock/gmock.h>

#include <userver/concurrent/async_event_channel.hpp>
#include <userver/engine/async.hpp>
#include <userver/engine/single_consumer_event.hpp>
#include <userver/engine/sleep.hpp>
#include <userver/utils/function_ref.hpp>
#include <userver/utils/not_null.hpp>
#include <userver/utils/resource_scopes.hpp>

USERVER_NAMESPACE_BEGIN

TEST(AsyncEventChannel, Ctr) { const concurrent::AsyncEventChannel<int> channel("channel"); }

namespace {

class Subscriber final {
public:
    Subscriber(
        utils::ResourceScopeStorage& scopes,
        concurrent::AsyncEventSource<int>& channel,
        utils::SharedRef<int> value
    )
        : value_(std::move(value))
    {
        channel.AddListener(scopes, this, "subscriber", &Subscriber::OnEvent);
    }

    void OnEvent(int value) { *value_ = value; }

private:
    const utils::SharedRef<int> value_;
};

class BlockingSubscriber final {
public:
    BlockingSubscriber(utils::ResourceScopeStorage& scopes, concurrent::AsyncEventSource<>& channel) {
        channel.AddListener(scopes, concurrent::FunctionId(this), "blocking", [this] {
            if (skip) {
                return;
            }
            inside_callback.Send();
            EXPECT_TRUE(may_exit.WaitForEvent());
        });
    }

    engine::SingleConsumerEvent inside_callback;
    engine::SingleConsumerEvent may_exit;
    std::atomic<bool> skip{false};
};

class CountingSubscriber final {
public:
    CountingSubscriber(utils::ResourceScopeStorage& scopes, concurrent::AsyncEventSource<>& channel) {
        channel.AddListener(scopes, concurrent::FunctionId(this), "counter", [this] { ++calls; });
    }

    std::atomic<std::size_t> calls{0};
};

}  // namespace

UTEST(AsyncEventChannel, Publish) {
    concurrent::AsyncEventChannel<int> channel("channel");
    const auto value = utils::MakeSharedRef<int>(0);
    EXPECT_EQ(*value, 0);

    utils::WithResourceScopes<Subscriber> sub(std::in_place, channel, value);
    EXPECT_EQ(*value, 0);

    channel.SendEvent(1);
    EXPECT_EQ(*value, 1);
}

UTEST(AsyncEventChannel, Unsubscribe) {
    concurrent::AsyncEventChannel<int> channel("channel");
    const auto value = utils::MakeSharedRef<int>(0);
    std::optional<utils::WithResourceScopes<Subscriber>> sub(std::in_place, std::in_place, channel, value);

    channel.SendEvent(1);
    EXPECT_EQ(*value, 1);

    sub.reset();
    channel.SendEvent(2);
    EXPECT_EQ(*value, 1);
}

UTEST(AsyncEventChannel, PublishTwoSubscribers) {
    concurrent::AsyncEventChannel<int> channel("channel");
    const auto value1 = utils::MakeSharedRef<int>(0);
    const auto value2 = utils::MakeSharedRef<int>(0);
    utils::WithResourceScopes<Subscriber> sub1(std::in_place, channel, value1);
    utils::WithResourceScopes<Subscriber> sub2(std::in_place, channel, value2);
    EXPECT_EQ(*value1, 0);
    EXPECT_EQ(*value2, 0);

    channel.SendEvent(1);
    EXPECT_EQ(*value1, 1);
    EXPECT_EQ(*value2, 1);
}

UTEST(AsyncEventChannel, PublishException) {
    concurrent::AsyncEventChannel<int> channel("channel");

    struct X {
        X(utils::ResourceScopeStorage& scopes, concurrent::AsyncEventSource<int>& source) {
            source.AddListener(scopes, this, "subscriber", &X::OnEvent);
        }

        // NOLINTNEXTLINE(readability-convert-member-functions-to-static)
        void OnEvent(int) { throw std::runtime_error("error msg"); }
    };
    utils::WithResourceScopes<X> sub(std::in_place, channel);
    UEXPECT_NO_THROW(channel.SendEvent(1));
}

UTEST(AsyncEventChannel, OnListenerRemoval) {
    int counter = 0;
    auto on_remove = [&counter](std::function<void(int)> func) {
        counter++;
        func(1);
    };
    concurrent::AsyncEventChannel<int> channel("channel", on_remove);

    const auto value1 = utils::MakeSharedRef<int>(0);
    const auto value2 = utils::MakeSharedRef<int>(0);
    const auto value3 = utils::MakeSharedRef<int>(0);
    utils::WithResourceScopes<Subscriber> sub1(std::in_place, channel, value1);
    {
        std::optional<utils::WithResourceScopes<Subscriber>> sub2(std::in_place, std::in_place, channel, value2);
        sub2.reset();
    }
    {
        utils::WithResourceScopes<Subscriber> sub3(std::in_place, channel, value3);
    }

    EXPECT_EQ(*value1, 0);
    if constexpr (concurrent::impl::kCheckSubscriptionUB) {
        EXPECT_EQ(*value2, 1);
        EXPECT_EQ(*value3, 1);
        EXPECT_EQ(counter, 2);
    } else {
        EXPECT_EQ(*value2, 0);
        EXPECT_EQ(*value3, 0);
        EXPECT_EQ(counter, 0);
    }
}

namespace {

/// [AsyncEventChannel sample]
enum class WeatherKind { kSunny, kRainy };

class WeatherStorage final {
public:
    explicit WeatherStorage(WeatherKind value)
        : value_(value),
          channel_("weather")
    {}

    WeatherKind Get() const { return value_.load(); }

    concurrent::AsyncEventSource<WeatherKind>& GetSource() { return channel_; }

    template <typename Class>
    void UpdateAndListen(
        utils::ResourceScopeStorage& scopes,
        Class* obj,
        std::string_view name,
        void (Class::*func)(WeatherKind)
    ) {
        channel_.DoUpdateAndListen(scopes, obj, name, func, [this, obj, func] { (obj->*func)(Get()); });
    }

    /// @overload
    /// @deprecated Use the overload that takes @ref utils::ResourceScopeStorage.
    template <typename Class>
    concurrent::AsyncEventSubscriberScope UpdateAndListen(
        Class* obj,
        std::string_view name,
        void (Class::*func)(WeatherKind)
    ) {
        return channel_.DoUpdateAndListen(obj, name, func, [&] { (obj->*func)(Get()); });
    }

    void Set(WeatherKind value) {
        value_.store(value);
        channel_.SendEvent(value);
    }

private:
    std::atomic<WeatherKind> value_;
    concurrent::AsyncEventChannel<WeatherKind> channel_;
};

enum class CoatKind { kJacket, kRaincoat };

class CoatStorage final {
public:
    explicit CoatStorage(utils::ResourceScopeStorage& scopes, WeatherStorage& weather_storage) {
        weather_storage.UpdateAndListen(scopes, this, "coats", &CoatStorage::OnWeatherUpdate);
    }

    CoatKind Get() const { return value_.load(); }

private:
    void OnWeatherUpdate(WeatherKind weather) { value_.store(ComputeCoat(weather)); }

    static CoatKind ComputeCoat(WeatherKind weather);

    std::atomic<CoatKind> value_{};
};

UTEST(AsyncEventChannel, UpdateAndListenSample) {
    WeatherStorage weather_storage(WeatherKind::kSunny);
    const utils::WithResourceScopes<CoatStorage> coat_storage(std::in_place, weather_storage);
    EXPECT_EQ(coat_storage->Get(), CoatKind::kJacket);
    weather_storage.Set(WeatherKind::kRainy);
    EXPECT_EQ(coat_storage->Get(), CoatKind::kRaincoat);
}
/// [AsyncEventChannel sample]

CoatKind CoatStorage::ComputeCoat(WeatherKind weather) {
    switch (weather) {
        case WeatherKind::kSunny:
            return CoatKind::kJacket;
        case WeatherKind::kRainy:
            return CoatKind::kRaincoat;
    }
    throw std::runtime_error("Invalid weather");
}

/// [AddListener sample]
class WeatherRecorder final {
public:
    WeatherRecorder(utils::ResourceScopeStorage& scopes, WeatherStorage& weather_storage) {
        weather_storage.GetSource()
            .AddListener(scopes, concurrent::FunctionId(this), "recorder", [this](WeatherKind weather) {
                recorded_weather.push_back(weather);
            });
    }

    std::vector<WeatherKind> recorded_weather;
};

UTEST(AsyncEventChannel, AddListenerSample) {
    WeatherStorage weather_storage(WeatherKind::kSunny);
    utils::WithResourceScopes<WeatherRecorder> recorder(std::in_place, weather_storage);

    weather_storage.Set(WeatherKind::kRainy);
    weather_storage.Set(WeatherKind::kSunny);
    weather_storage.Set(WeatherKind::kSunny);
    EXPECT_EQ(recorder->recorded_weather, (std::vector{WeatherKind::kRainy, WeatherKind::kSunny, WeatherKind::kSunny}));
}
/// [AddListener sample]

class ScopedListener final {
public:
    ScopedListener(utils::ResourceScopeStorage& scopes, concurrent::AsyncEventChannel<int>& channel)
        : channel_(channel)
    {
        concurrent::AsyncEventSource<int>& source = channel_;
        source.AddListener(scopes, this, std::string{"subscriber"}, &ScopedListener::OnEvent);
        channel_.SendEvent(1);
        EXPECT_EQ(value, 0);
    }

    ~ScopedListener() {
        const auto previous = value;
        channel_.SendEvent(3);
        EXPECT_EQ(value, previous);
    }

    void OnEvent(int event) { value = event; }

    int value{0};

private:
    concurrent::AsyncEventChannel<int>& channel_;
};

UTEST(AsyncEventChannel, ScopedListenerLifetime) {
    concurrent::AsyncEventChannel<int> channel("channel");
    {
        utils::WithResourceScopes<ScopedListener> subscriber(std::in_place, channel);
        EXPECT_EQ(subscriber->value, 0);
        channel.SendEvent(2);
        EXPECT_EQ(subscriber->value, 2);
    }
    channel.SendEvent(4);
}

UTEST(AsyncEventChannel, OnListenerRemovalSample) {
    /*! [OnListenerRemoval sample] */
    auto on_remove = [](std::function<void(int)> func) { func(1); };

    concurrent::AsyncEventChannel<int> channel("channel", on_remove);

    struct Recorder {
        Recorder(
            utils::ResourceScopeStorage& scopes,
            concurrent::AsyncEventSource<int>& source,
            utils::SharedRef<int> shared_value
        )
            : value(std::move(shared_value))
        {
            source.AddListener(scopes, concurrent::FunctionId(this), "recorder", [this](int new_value) {
                *value = new_value;
            });
        }

        const utils::SharedRef<int> value;
    };

    const auto value = utils::MakeSharedRef<int>(0);
    {
        utils::WithResourceScopes<Recorder> sub(std::in_place, channel, value);
    }

    if constexpr (concurrent::impl::kCheckSubscriptionUB) {
        EXPECT_EQ(*value, 1);
    } else {
        EXPECT_EQ(*value, 0);
    }
    /*! [OnListenerRemoval sample] */
}

UTEST(AsyncEventChannel, SendEventConcurrent) {
    concurrent::AsyncEventChannel<> channel("channel");
    utils::WithResourceScopes<BlockingSubscriber> sub(std::in_place, channel);
    utils::WithResourceScopes<CountingSubscriber> sub2(std::in_place, channel);

    auto first_task = engine::AsyncNoTracing([&channel] { channel.SendEvent(); });
    EXPECT_TRUE(sub->inside_callback.WaitForEvent());
    sub->skip = true;

    auto task1 = engine::AsyncNoTracing([&channel] { channel.SendEvent(); });
    auto task2 = engine::AsyncNoTracing([&channel] { channel.SendEvent(); });

    // Make sure task1 & task2 are waiting on SharedMutex::lock().
    engine::SleepFor(std::chrono::milliseconds(100));
    sub->may_exit.Send();

    first_task.Get();
    task1.Get();
    task2.Get();
}

UTEST(AsyncEventChannel, SendEventConcurrent2) {
    concurrent::AsyncEventChannel<> channel("channel");
    std::optional<utils::WithResourceScopes<BlockingSubscriber>> sub(std::in_place, std::in_place, channel);
    auto& subscriber = *sub.value();
    utils::WithResourceScopes<CountingSubscriber> sub2(std::in_place, channel);

    auto first_task = engine::AsyncNoTracing([&channel] { channel.SendEvent(); });
    EXPECT_TRUE(subscriber.inside_callback.WaitForEvent());

    auto may_exit_task = engine::AsyncNoTracing([&subscriber] { subscriber.may_exit.Send(); });
    may_exit_task.Get();
    sub.reset();

    auto task1 = engine::AsyncNoTracing([&channel] { channel.SendEvent(); });
    auto task2 = engine::AsyncNoTracing([&channel] { channel.SendEvent(); });

    first_task.Get();
    task1.Get();
    task2.Get();
    EXPECT_EQ(sub2->calls.load(), 3);
}

class ScopedSubscriber final {
public:
    ScopedSubscriber(
        utils::ResourceScopeStorage& scopes,
        concurrent::AsyncEventChannel<int>& channel,
        utils::SharedRef<std::vector<int>> updates,
        const std::atomic<int>& current,
        utils::function_ref<void()> after_subscribe
    )
        : current_(current),
          updates_(std::move(updates))
    {
        channel.DoUpdateAndListen(scopes, this, "sub", &ScopedSubscriber::OnEvent, [this] {
            OnEvent(current_.load());
        });
        after_subscribe();
    }

private:
    void OnEvent(int value) { updates_->push_back(value); }

    const std::atomic<int>& current_;
    const utils::SharedRef<std::vector<int>> updates_;
};

UTEST(AsyncEventChannel, DoUpdateAndListenCallsUpdaterOnceWithoutEvents) {
    concurrent::AsyncEventChannel<int> channel("channel");
    const auto updates = utils::MakeSharedRef<std::vector<int>>();
    const std::atomic<int> current{1};
    auto subscriber = utils::MakeWithResourceScopes<ScopedSubscriber>(channel, updates, current, [&] {
        EXPECT_THAT(*updates, ::testing::ElementsAre(1));
    });
    EXPECT_THAT(*updates, ::testing::ElementsAre(1));

    channel.SendEvent(2);
    EXPECT_THAT(*updates, ::testing::ElementsAre(1, 2));

    subscriber.reset();
    channel.SendEvent(3);
    EXPECT_THAT(*updates, ::testing::ElementsAre(1, 2));
}

UTEST(AsyncEventChannel, DoUpdateAndListenReplaysUpdaterIfEventSkipped) {
    concurrent::AsyncEventChannel<int> channel("channel");
    const auto updates = utils::MakeSharedRef<std::vector<int>>();
    std::atomic<int> current{0};
    {
        const utils::WithResourceScopes<ScopedSubscriber>
            subscriber{std::in_place, channel, updates, current, [&] {
                           EXPECT_THAT(*updates, ::testing::ElementsAre(0));
                           current = 5;
                           channel.SendEvent(5);
                           EXPECT_THAT(*updates, ::testing::ElementsAre(0));
                       }};
        EXPECT_THAT(*updates, ::testing::ElementsAre(0, 5));

        current = 7;
        channel.SendEvent(7);
        EXPECT_THAT(*updates, ::testing::ElementsAre(0, 5, 7));
    }
    channel.SendEvent(8);
    EXPECT_THAT(*updates, ::testing::ElementsAre(0, 5, 7));
}

UTEST(AsyncEventChannel, DoUpdateAndListenUnsubscribesIfConstructionFailed) {
    concurrent::AsyncEventChannel<int> channel("channel");
    const auto updates = utils::MakeSharedRef<std::vector<int>>();
    std::atomic<int> current{1};
    UEXPECT_THROW(
        utils::MakeWithResourceScopes<ScopedSubscriber>(
            channel,
            updates,
            current,
            [&] {
                EXPECT_THAT(*updates, ::testing::ElementsAre(1));
                current = 2;
                channel.SendEvent(2);
                EXPECT_THAT(*updates, ::testing::ElementsAre(1));
                throw std::runtime_error("subscriber construction failed");
            }
        ),
        std::runtime_error
    );
    channel.SendEvent(3);
    EXPECT_THAT(*updates, ::testing::ElementsAre(1));
}

UTEST(AsyncEventChannel, UnsibscribeWhileHandling) {
    struct SlowSubscriber {
        SlowSubscriber(
            utils::ResourceScopeStorage& scopes,
            concurrent::AsyncEventSource<>& channel,
            utils::SharedRef<std::atomic<bool>> finished
        )
            : unsubscribe_has_finished(std::move(finished))
        {
            channel.AddListener(scopes, concurrent::FunctionId(this), "test", [this] {
                started.Send();
                engine::SleepFor(std::chrono::milliseconds(5));
                EXPECT_FALSE(unsubscribe_has_finished->load());
            });
        }

        engine::SingleConsumerEvent started;
        const utils::SharedRef<std::atomic<bool>> unsubscribe_has_finished;
    };

    concurrent::AsyncEventChannel<> channel("channel");
    const auto unsubscribe_has_finished = utils::MakeSharedRef<std::atomic<bool>>(false);
    std::optional<utils::WithResourceScopes<SlowSubscriber>>
        sub(std::in_place, std::in_place, channel, unsubscribe_has_finished);
    auto send_task = engine::AsyncNoTracing([&channel] { channel.SendEvent(); });
    EXPECT_TRUE(sub.value()->started.WaitForEvent());

    sub.reset();
    *unsubscribe_has_finished = true;
    send_task.Get();
}

}  // namespace

USERVER_NAMESPACE_END
