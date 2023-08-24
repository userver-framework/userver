#include <userver/storages/redis/mock_client_google.hpp>

USERVER_NAMESPACE_BEGIN

namespace storages::redis::test {

TEST(MockClientTest, Basic) {
  auto client_mock = std::make_shared<GMockClient>();

  using testing::_;

  EXPECT_CALL(*client_mock, ShardByKey("test_shard"))
      .Times(1)
      .WillRepeatedly([](const std::string&) { return 1; });

  size_t shardCount = client_mock->ShardByKey("test_shard");
  ASSERT_EQ(shardCount, 1);
}

}  // namespace storages::redis::test

USERVER_NAMESPACE_END
