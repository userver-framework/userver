#include <userver/storages/redis/mock_request.hpp>
#include <userver/storages/redis/request.hpp>

USERVER_NAMESPACE_BEGIN

TEST(ScanRequest, PostfixIncrementCorrect) {
  auto scan_request =
      storages::redis::CreateMockRequestScan<storages::redis::ScanTag::kScan>(
          {"1", "2"});
  auto it = scan_request.begin();
  EXPECT_EQ(*it, "1");
  it++;
  EXPECT_EQ(*it, "2");
  it++;
  EXPECT_EQ(it, scan_request.end());
}

USERVER_NAMESPACE_END
