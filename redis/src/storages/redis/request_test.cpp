#include <userver/storages/redis/mock_request.hpp>
#include <userver/storages/redis/request.hpp>

using namespace USERVER_NAMESPACE::storages::redis;

TEST(ScanRequest, PostfixIncrementCorrect) {
  auto scan_request = CreateMockRequestScan<ScanTag::kScan>({"1", "2"});
  auto it = scan_request.begin();
  EXPECT_EQ(*it, "1");
  it++;
  EXPECT_EQ(*it, "2");
  it++;
  EXPECT_EQ(it, scan_request.end());
}
