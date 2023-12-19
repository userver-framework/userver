#pragma once

#include <string>

#include <userver/engine/single_consumer_event.hpp>
#include <userver/logging/log.hpp>
#include <userver/storages/redis/mock_client_google.hpp>
#include <userver/utest/utest.hpp>

USERVER_NAMESPACE_BEGIN

namespace storages::redis {

// This is very simple waiter class. It expects one - and exactly one
// Publish call on redis gmock that should match given matcher. And calling
// Wait() will ensure that this call did indeed happened.
// Basic usage is like this:
// auto my_redis_mock = GetRedisMock();
// MockPublishWaiter waiter(
//     my_redis_mock,
//     "my-channel",
//     StartsWith("my-channel"));
//
// DoSomeStuffThatPublishesToChannel(my_redis_mock);
//
// // Wait for successful detection of Publish call
// my_redis_mock.Wait();
//
// You may want some more complicated checks or behaviours. It such a case, feel
// free to copy this class and adapt it for your needs.
class MockPublishWaiter {
 public:
  // channel_name is used as PREFIX! and not as exact match
  template <typename T>
  MockPublishWaiter(GMockClient& redis_mock,
                    const std::string& debug_channel_name, T&& matcher,
                    size_t times_called = 1)
      : debug_channel_name_(std::move(debug_channel_name)) {
    UINVARIANT(times_called > 0, "times_called must be > 0");

    using ::testing::_;
    EXPECT_CALL(redis_mock, Publish(std::forward<T>(matcher), _, _, _))
        .Times(times_called)
        .WillRepeatedly([this, times_called](auto&& channel, auto&&...) {
          LOG_DEBUG() << "Redis mock received message to channel: " << channel
                      << "; Called time: " << times_called_.load() + 1;
          if ((times_called_.fetch_add(1) + 1) == times_called) {
            on_received_.Send();
          }
        });
  }

  void Wait() {
    EXPECT_TRUE(on_received_.WaitForEventFor(std::chrono::seconds{30}))
        << "Failed to detect publishing to channel " << debug_channel_name_
        << " within 30 seconds";
  }

 private:
  engine::SingleConsumerEvent on_received_;
  std::string debug_channel_name_;
  std::atomic<size_t> times_called_{0};
};

}  // namespace storages::redis

USERVER_NAMESPACE_END
