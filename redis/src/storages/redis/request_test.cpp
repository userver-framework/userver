#include <userver/storages/redis/mock_request.hpp>
#include <userver/storages/redis/request.hpp>

#include <iterator>

USERVER_NAMESPACE_BEGIN

static_assert(std::input_iterator<storages::redis::ScanRequest<storages::redis::ScanTag::kScan>::Iterator>);

TEST(ScanRequest, PostfixIncrementCorrect) {
    auto scan_request = storages::redis::CreateMockRequestScan<storages::redis::ScanTag::kScan>({"1", "2"});
    auto it = scan_request.begin();
    EXPECT_EQ(*it, "1");
    it++;
    EXPECT_EQ(*it, "2");
    it++;
    EXPECT_EQ(it, scan_request.end());
}

TEST(ScanRequest, ConstIteratorDereference) {
    auto scan_request = storages::redis::CreateMockRequestScan<storages::redis::ScanTag::kScan>({"abc", "def"});
    const auto it = scan_request.begin();
    EXPECT_EQ(*it, "abc");
    EXPECT_EQ(it->size(), 3);
}

USERVER_NAMESPACE_END
