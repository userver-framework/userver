#include <userver/storages/redis/mock_publish_waiter.hpp>

USERVER_NAMESPACE_BEGIN

namespace storages::redis {

UTEST(MockPublishWaiter, TestBasic) {
    auto my_redis_mock = std::make_shared<GMockClient>();

    MockPublishWaiter waiter(*my_redis_mock, "test-channel", ::testing::StartsWith("test-ch"));

    my_redis_mock->Publish("test-channel", "test_data", {}, {});

    waiter.Wait();

    ASSERT_EQ(waiter.GetPublishedMessagesCount(), 1);
    ASSERT_EQ(waiter.GetPublishedMessage(0), "test_data");
}

}  // namespace storages::redis

USERVER_NAMESPACE_END
