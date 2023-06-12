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

  redis::CommandControl cc{GetParam()};
  auto token =
      GetSubscribeClient()->Subscribe(test_channel, std::move(callback), cc);

  std::chrono::seconds deadwait{15};
  EXPECT_TRUE(success.WaitForEventFor(deadwait))
      << "Couldn't receive message for " << deadwait.count() << " seconds";

  sender.RequestCancel();
  token.Unsubscribe();
}

UTEST_P_MT(RedisPubsubTestBasic, SimplePsubscribe, 2) {
  const std::string test_data = "something_else";
  const std::string test_channel = "interior";
  const std::string test_pattern = "in*";

  engine::SingleConsumerEvent success;

  auto callback = [&](const std::string& pattern, const std::string& channel,
                      const std::string& data) {
    if (channel == test_channel && data == test_data &&
        pattern == test_pattern) {
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

  redis::CommandControl cc{GetParam()};
  auto token =
      GetSubscribeClient()->Psubscribe(test_pattern, std::move(callback), cc);

  std::chrono::seconds deadwait{15};
  EXPECT_TRUE(success.WaitForEventFor(deadwait))
      << "Couldn't receive message for " << deadwait.count() << " seconds";

  sender.RequestCancel();
  token.Unsubscribe();
}

namespace {

std::vector<redis::CommandControl> BuildTestData() {
  std::vector<redis::CommandControl> result;

  // One default CC
  result.emplace_back(redis::CommandControl{});

  // Add one with queue enabled. It most likely is default behaviour, however
  // explicit test is always better
  {
    redis::CommandControl cc;
    cc.disable_subscription_queueing = false;
    result.emplace_back(std::move(cc));
  }

  // Add one with queue disabled.
  {
    redis::CommandControl cc;
    cc.disable_subscription_queueing = true;
    result.emplace_back(std::move(cc));
  }

  // Add one with queue disabled.
  {
    redis::CommandControl cc;
    cc.disable_subscription_queueing = true;
    result.emplace_back(std::move(cc));
  }

  // add more CC test cases below
  // ....

  return result;
}

const std::vector<redis::CommandControl>& TestData() {
  // C++11 standard ensures that initialization will be thread-safe
  static std::vector<redis::CommandControl> test_data{BuildTestData()};
  return test_data;
}

}  // namespace

INSTANTIATE_UTEST_SUITE_P(BasicSequence, RedisPubsubTestBasic,
                          ::testing::ValuesIn(TestData()));

USERVER_NAMESPACE_END
