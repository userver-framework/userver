#include <userver/utest/utest.hpp>

#include <atomic>
#include <stdexcept>

#include <userver/concurrent/async_event_channel.hpp>
#include <userver/engine/single_consumer_event.hpp>
#include <userver/engine/sleep.hpp>

USERVER_NAMESPACE_BEGIN

TEST(AsyncEventChannel, Ctr) { const concurrent::AsyncEventChannel<int> channel("channel"); }

namespace {

class Subscriber final {
public:
    Subscriber(int& x)
        : x_(x)
    {}

    void OnEvent(int x) { x_ = x; }

private:
    int& x_;
};

}  // namespace

UTEST(AsyncEventChannel, Publish) {
    concurrent::AsyncEventChannel<int> channel("channel");

    int value{0};
    Subscriber s(value);
    EXPECT_EQ(value, 0);

    auto sub = channel.AddListener(&s, "sub", &Subscriber::OnEvent);
    EXPECT_EQ(value, 0);

    channel.SendEvent(1);
    EXPECT_EQ(value, 1);

    sub.Unsubscribe();
}

UTEST(AsyncEventChannel, Unsubscribe) {
    concurrent::AsyncEventChannel<int> channel("channel");

    int value{0};
    Subscriber s(value);
    auto sub = channel.AddListener(&s, "", &Subscriber::OnEvent);

    channel.SendEvent(1);
    EXPECT_EQ(value, 1);

    sub.Unsubscribe();
    channel.SendEvent(2);
    EXPECT_EQ(value, 1);
}

UTEST(AsyncEventChannel, PublishTwoSubscribers) {
    concurrent::AsyncEventChannel<int> channel("channel");

    int value1{0};
    int value2{0};
    Subscriber s1(value1);
    Subscriber s2(value2);

    auto sub1 = channel.AddListener(&s1, "", &Subscriber::OnEvent);
    auto sub2 = channel.AddListener(&s2, "", &Subscriber::OnEvent);
    EXPECT_EQ(value1, 0);
    EXPECT_EQ(value2, 0);

    channel.SendEvent(1);
    EXPECT_EQ(value1, 1);
    EXPECT_EQ(value2, 1);

    sub1.Unsubscribe();
    sub2.Unsubscribe();
}

UTEST(AsyncEventChannel, PublishException) {
    concurrent::AsyncEventChannel<int> channel("channel");

    struct X {
        // NOLINTNEXTLINE(readability-convert-member-functions-to-static)
        void OnEvent(int) { throw std::runtime_error("error msg"); }
    };
    X x;

    auto sub1 = channel.AddListener(&x, "subscriber", &X::OnEvent);
    UEXPECT_NO_THROW(channel.SendEvent(1));
    sub1.Unsubscribe();
}

UTEST(AsyncEventChannel, OnListenerRemoval) {
    int counter = 0;
    auto on_remove = [&counter](std::function<void(int)> func) {
        counter++;
        func(1);
    };
    concurrent::AsyncEventChannel<int> channel("channel", on_remove);

    int value1{0};
    int value2{0};
    Subscriber s1(value1);
    auto sub1 = channel.AddListener(&s1, "", &Subscriber::OnEvent);
    {
        Subscriber s2(value2);
        auto sub2 = channel.AddListener(&s2, "", &Subscriber::OnEvent);
    }

    EXPECT_EQ(value1, 0);
    if constexpr (concurrent::impl::kCheckSubscriptionUB) {
        EXPECT_EQ(value2, 1);
        EXPECT_EQ(counter, 1);
    } else {
        EXPECT_EQ(value2, 0);
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
    explicit CoatStorage(WeatherStorage& weather_storage) {
        weather_subscriber_ = weather_storage.UpdateAndListen(this, "coats", &CoatStorage::OnWeatherUpdate);
    }

    ~CoatStorage() { weather_subscriber_.Unsubscribe(); }

    CoatKind Get() const { return value_.load(); }

private:
    void OnWeatherUpdate(WeatherKind weather) { value_.store(ComputeCoat(weather)); }

    static CoatKind ComputeCoat(WeatherKind weather);

    std::atomic<CoatKind> value_{};
    concurrent::AsyncEventSubscriberScope weather_subscriber_;
};

UTEST(AsyncEventChannel, UpdateAndListenSample) {
    WeatherStorage weather_storage(WeatherKind::kSunny);
    const CoatStorage coat_storage(weather_storage);
    EXPECT_EQ(coat_storage.Get(), CoatKind::kJacket);
    weather_storage.Set(WeatherKind::kRainy);
    EXPECT_EQ(coat_storage.Get(), CoatKind::kRaincoat);
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
UTEST(AsyncEventChannel, AddListenerSample) {
    WeatherStorage weather_storage(WeatherKind::kSunny);
    std::vector<WeatherKind> recorded_weather;

    concurrent::AsyncEventSubscriberScope recorder =
        weather_storage.GetSource()
            .AddListener(concurrent::FunctionId(&recorder), "recorder", [&](WeatherKind weather) {
                recorded_weather.push_back(weather);
            });

    weather_storage.Set(WeatherKind::kRainy);
    weather_storage.Set(WeatherKind::kSunny);
    weather_storage.Set(WeatherKind::kSunny);
    EXPECT_EQ(recorded_weather, (std::vector{WeatherKind::kRainy, WeatherKind::kSunny, WeatherKind::kSunny}));

    recorder.Unsubscribe();
}
/// [AddListener sample]

UTEST(AsyncEventChannel, OnListenerRemovalSample) {
    /*! [OnListenerRemoval sample] */
    auto on_remove = [](std::function<void(int)> func) { func(1); };

    concurrent::AsyncEventChannel<int> channel("channel", on_remove);

    int value = 0;
    {
        concurrent::AsyncEventSubscriberScope sub =
            channel.AddListener(concurrent::FunctionId(&sub), "sub", [&value](int new_value) { value = new_value; });
    }

    if constexpr (concurrent::impl::kCheckSubscriptionUB) {
        EXPECT_EQ(value, 1);
    } else {
        EXPECT_EQ(value, 0);
    }
    /*! [OnListenerRemoval sample] */
}

UTEST(AsyncEventChannel, SendEventConcurrent) {
    concurrent::AsyncEventChannel<> channel("channel");
    engine::SingleConsumerEvent inside_callback;
    engine::SingleConsumerEvent may_exit;
    std::atomic<bool> skip{false};

    auto sub = channel.AddListener(concurrent::FunctionId(&channel), "test", [&] {
        if (skip) {
            return;
        }

        inside_callback.Send();
        EXPECT_TRUE(may_exit.WaitForEvent());
    });

    // This callback invocation may result in deadlock
    auto sub2 = channel.AddListener(concurrent::FunctionId(&may_exit), "test2", [] {});

    auto first_task = engine::AsyncNoSpan([&] { channel.SendEvent(); });

    EXPECT_TRUE(inside_callback.WaitForEvent());

    skip = true;

    auto task1 = engine::AsyncNoSpan([&] { channel.SendEvent(); });
    auto task2 = engine::AsyncNoSpan([&] { channel.SendEvent(); });

    // Make sure task1 & task2 are waiting on SharedMutex::lock()
    engine::SleepFor(std::chrono::milliseconds(100));

    // One task is inside critical section while another is still waiting
    // on SharedMutex::lock()
    may_exit.Send();

    first_task.Get();
}

UTEST(AsyncEventChannel, SendEventConcurrent2) {
    concurrent::AsyncEventChannel<> channel("channel");
    engine::SingleConsumerEvent inside_callback;
    engine::SingleConsumerEvent may_exit;

    auto sub = channel.AddListener(concurrent::FunctionId(&channel), "test", [&] {
        inside_callback.Send();
        EXPECT_TRUE(may_exit.WaitForEvent());
    });

    std::atomic<std::size_t> calls{0};
    auto sub2 = channel.AddListener(concurrent::FunctionId(&may_exit), "test2", [&calls] { ++calls; });

    auto first_task = engine::AsyncNoSpan([&] { channel.SendEvent(); });

    EXPECT_TRUE(inside_callback.WaitForEvent());

    auto may_exit_task = engine::AsyncNoSpan([&] { may_exit.Send(); });
    sub.Unsubscribe();

    auto task1 = engine::AsyncNoSpan([&] { channel.SendEvent(); });
    auto task2 = engine::AsyncNoSpan([&] { channel.SendEvent(); });

    first_task.Get();
    task1.Get();
    task2.Get();
    EXPECT_EQ(calls.load(), 3);
}

UTEST(AsyncEventChannel, UnsibscribeWhileHandling) {
    engine::SingleConsumerEvent started;
    std::atomic<bool> unsubscribe_has_finished{false};
    concurrent::AsyncEventChannel<> channel("channel");

    auto sub = channel.AddListener(concurrent::FunctionId(&channel), "test", [&] {
        started.Send();
        engine::SleepFor(std::chrono::milliseconds(5));
        EXPECT_FALSE(unsubscribe_has_finished);
    });

    auto send_task = engine::AsyncNoSpan([&] { channel.SendEvent(); });
    (void)started.WaitForEvent();

    sub.Unsubscribe();
    unsubscribe_has_finished = true;
}

}  // namespace

USERVER_NAMESPACE_END
