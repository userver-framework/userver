#include <storages/redis/pubsub_redistest.hpp>

#include <algorithm>
#include <memory>
#include <string_view>
#include <tuple>
#include <vector>

#include <userver/engine/single_consumer_event.hpp>
#include <userver/engine/sleep.hpp>
#include <userver/engine/task/task.hpp>
#include <userver/utils/async.hpp>

USERVER_NAMESPACE_BEGIN

UTEST_P_MT(RedisPubsubTestBasic, SimpleSubscribe, 2) {
    const std::string test_data = "something_else";
    const std::string test_channel = "interior";

    engine::SingleConsumerEvent success;

    auto callback = [&](const std::string& channel, const std::string& data) {
        if (channel == test_channel && data == test_data) {
            success.Send();
        }
    };

    // We don't really trust that redis pubsub is reliable - even when
    // launched locally and in unit test environment. So, we will launch coroutine
    // that constantly sends messages to redis and then we will subscribe to redis
    // and wait for success.
    auto sender = utils::CriticalAsync("sender", [&]() {
        while (!engine::current_task::ShouldCancel()) {
            GetClient()->Publish(test_channel, test_data, {});
            engine::InterruptibleSleepFor(std::chrono::seconds{1});
        }
    });

    const storages::redis::CommandControl cc{GetParam()};
    auto token = GetSubscribeClient()->Subscribe(test_channel, std::move(callback), cc);

    EXPECT_TRUE(success.WaitForEventFor(utest::kMaxTestWaitTime));

    sender.RequestCancel();
    token.Unsubscribe();
}

UTEST_P_MT(RedisPubsubTestBasic, MultiChannelSubscribe, 2) {
    const std::string test_data = "something_else";
    const std::vector<std::string> test_channels = {"interior", "other_channel"};
    const storages::redis::CommandControl cc{GetParam()};

    engine::SingleConsumerEvent success;

    std::string expected_channel;
    auto callback = [&](const std::string& channel, const std::string& data) {
        if (channel == expected_channel && data == test_data) {
            success.Send();
        }
    };
    // subscribe one time for both channels
    auto token = GetSubscribeClient()->Subscribe(test_channels, std::move(callback), cc);

    for (const auto& test_channel : test_channels) {
        expected_channel = test_channel;
        // We don't really trust that redis pubsub is reliable - even when
        // launched locally and in unit test environment. So, we will launch coroutine
        // that constantly sends messages to redis and then we will subscribe to redis
        // and wait for success.
        auto sender = utils::CriticalAsync("sender", [&]() {
            while (!engine::current_task::ShouldCancel()) {
                GetClient()->Publish(test_channel, test_data, {});
                engine::InterruptibleSleepFor(std::chrono::seconds{1});
            }
        });

        EXPECT_TRUE(success.WaitForEventFor(utest::kMaxTestWaitTime));

        sender.RequestCancel();
    }
    token.Unsubscribe();
}

UTEST_P_MT(RedisPubsubTestBasic, SimplePsubscribe, 2) {
    const std::string test_data = "something_else";
    const std::string test_channel = "interior";
    const std::string test_pattern = "in*";

    engine::SingleConsumerEvent success;

    auto callback = [&](const std::string& pattern, const std::string& channel, const std::string& data) {
        if (channel == test_channel && data == test_data && pattern == test_pattern) {
            success.Send();
        }
    };

    // We don't really trust that redis pubsub is reliable - even when
    // launched locally and in unit test environment. So, we will launch coroutine
    // that constantly sends messages to redis and then we will subscribe to redis
    // and wait for success.
    auto sender = utils::CriticalAsync("sender", [&]() {
        while (!engine::current_task::ShouldCancel()) {
            GetClient()->Publish(test_channel, test_data, {});
            engine::InterruptibleSleepFor(std::chrono::seconds{1});
        }
    });

    const storages::redis::CommandControl cc{GetParam()};
    auto token = GetSubscribeClient()->Psubscribe(test_pattern, std::move(callback), cc);

    EXPECT_TRUE(success.WaitForEventFor(utest::kMaxTestWaitTime));

    sender.RequestCancel();
    token.Unsubscribe();
}

UTEST_P_MT(RedisPubsubTestBasic, MultiPatternPsubscribe, 2) {
    const std::string test_data = "something_else";
    const std::string test_channel = "interior";
    /// pairs : pattern, channel
    const std::vector<std::pair<std::string, std::string>> test_cases = {
        {"in*", "interior"},
        {"oth*", "other_channel"},
    };
    const std::vector<std::string> test_patterns = {"in*", "oth*"};

    engine::SingleConsumerEvent success;
    std::string expected_channel;
    std::string expected_pattern;
    auto callback = [&](const std::string& pattern, const std::string& channel, const std::string& data) {
        if (channel == expected_channel && data == test_data && pattern == expected_pattern) {
            success.Send();
        }
    };
    const storages::redis::CommandControl cc{GetParam()};
    // single subscription for both cases
    auto token = GetSubscribeClient()->Psubscribe(test_patterns, std::move(callback), cc);

    for (const auto& [test_pattern, test_channel] : test_cases) {
        expected_channel = test_channel;
        expected_pattern = test_pattern;
        // We don't really trust that redis pubsub is reliable - even when
        // launched locally and in unit test environment. So, we will launch coroutine
        // that constantly sends messages to redis and then we will subscribe to redis
        // and wait for success.
        auto sender = utils::CriticalAsync("sender", [&]() {
            while (!engine::current_task::ShouldCancel()) {
                GetClient()->Publish(test_channel, test_data, {});
                engine::InterruptibleSleepFor(std::chrono::seconds{1});
            }
        });

        EXPECT_TRUE(success.WaitForEventFor(utest::kMaxTestWaitTime));

        sender.RequestCancel();
    }
    token.Unsubscribe();
}

UTEST_P_MT(RedisClusterPubsubTestBasic, SimpleSsubscribe, 2) {
// hiredis does not properly support `ssubscribe` command.
//
// libvalkey supports from first release
// https://github.com/valkey-io/libvalkey/commit/88b214d372005aa046adac8b1cafd10f76e89f58
// but we do not use libvalkey yet.
#ifdef ARCADIA_ROOT
    const std::string test_data = "something_else";
    const std::string test_channel = "interior";

    engine::SingleConsumerEvent success;

    auto callback = [&](const std::string& channel, const std::string& data) {
        if (channel == test_channel && data == test_data) {
            success.Send();
        }
    };

    // We don't really trust that redis pubsub is reliable - even when
    // launched locally and in unit test environment. So, we will launch coroutine
    // that constantly sends messages to redis and then we will subscribe to redis
    // and wait for success.
    auto sender = utils::CriticalAsync("sender", [&]() {
        while (!engine::current_task::ShouldCancel()) {
            GetClient()->Spublish(test_channel, test_data, {});
            engine::InterruptibleSleepFor(std::chrono::seconds{1});
        }
    });

    const storages::redis::CommandControl cc{GetParam()};
    auto token = GetSubscribeClient()->Ssubscribe(test_channel, std::move(callback), cc);

    EXPECT_TRUE(success.WaitForEventFor(utest::kMaxTestWaitTime));

    sender.RequestCancel();
    token.Unsubscribe();
#endif
}

UTEST_P_MT(RedisClusterPubsubTestBasic, MultiChannelSsubscribe, 2) {
// hiredis does not properly support `ssubscribe` command.
//
// libvalkey supports from first release
// https://github.com/valkey-io/libvalkey/commit/88b214d372005aa046adac8b1cafd10f76e89f58
// but we do not use libvalkey yet.
#ifdef ARCADIA_ROOT
    const std::string test_data = "something_else";
    const std::vector<std::string> test_channels = {"interior", "other_channel"};
    const storages::redis::CommandControl cc{GetParam()};

    engine::SingleConsumerEvent success;
    std::string expected_cahnnel;
    auto callback = [&](const std::string& channel, const std::string& data) {
        if (channel == expected_cahnnel && data == test_data) {
            success.Send();
        }
    };
    // subscribe one time for both channels
    auto token = GetSubscribeClient()->Ssubscribe(test_channels, std::move(callback), cc);

    for (const auto& test_channel : test_channels) {
        expected_cahnnel = test_channel;
        // We don't really trust that redis pubsub is reliable - even when
        // launched locally and in unit test environment. So, we will launch coroutine
        // that constantly sends messages to redis and then we will subscribe to redis
        // and wait for success.
        auto sender = utils::CriticalAsync("sender", [&]() {
            while (!engine::current_task::ShouldCancel()) {
                GetClient()->Spublish(test_channel, test_data, {});
                engine::InterruptibleSleepFor(std::chrono::seconds{1});
            }
        });

        EXPECT_TRUE(success.WaitForEventFor(utest::kMaxTestWaitTime));

        sender.RequestCancel();
    }
    token.Unsubscribe();

#endif
}

namespace {

std::vector<storages::redis::CommandControl> BuildTestData() {
    std::vector<storages::redis::CommandControl> result;

    // One default CC
    result.emplace_back();

    return result;
}

const std::vector<storages::redis::CommandControl>& TestData() {
    // C++11 standard ensures that initialization will be thread-safe
    static const std::vector<storages::redis::CommandControl> kTestData{BuildTestData()};
    return kTestData;
}

}  // namespace

INSTANTIATE_UTEST_SUITE_P(BasicSequence, RedisPubsubTestBasic, ::testing::ValuesIn(TestData()));
INSTANTIATE_UTEST_SUITE_P(BasicSequence, RedisClusterPubsubTestBasic, ::testing::ValuesIn(TestData()));

USERVER_NAMESPACE_END
