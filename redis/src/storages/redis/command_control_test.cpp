#include <userver/storages/redis/command_control.hpp>

#include <userver/utest/utest.hpp>

USERVER_NAMESPACE_BEGIN

#ifdef USERVER_IMPL_HAS_THREE_WAY_COMPARISON
TEST(CommandControl, Comparison) {
    const storages::redis::CommandControl cc1{std::chrono::milliseconds(500), std::chrono::milliseconds(500), 1};
    const storages::redis::CommandControl cc2{std::chrono::milliseconds(500), std::chrono::milliseconds(500), 2};
    auto cc3 = cc1;
    cc3.force_request_to_master = true;

    EXPECT_NE(cc1, cc2);
    EXPECT_NE(cc1, cc3);
    EXPECT_EQ(cc1, cc1);
    EXPECT_EQ(cc2, cc2);
    EXPECT_EQ(cc3, cc3);
}
#endif

USERVER_NAMESPACE_END
