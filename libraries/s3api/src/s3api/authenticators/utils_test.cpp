#include <userver/s3api/authenticators/utils.hpp>

#include <userver/utils/mock_now.hpp>

#include <userver/utest/utest.hpp>

USERVER_NAMESPACE_BEGIN

namespace s3api::authenticators {

TEST(S3ApiUtils, MakeHeaderDate) {
    const auto kMockTime = std::chrono::system_clock::from_time_t(1567534400);
    utils::datetime::MockNowSet(kMockTime);
    EXPECT_EQ("Tue, 03 Sep 2019 18:13:20 +0000", s3api::authenticators::MakeHeaderDate());
}

}  // namespace s3api::authenticators

USERVER_NAMESPACE_END
