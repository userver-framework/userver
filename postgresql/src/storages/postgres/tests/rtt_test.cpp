#include <userver/utest/utest.hpp>

#include <chrono>

#include <storages/postgres/detail/rtt.hpp>

USERVER_NAMESPACE_BEGIN

namespace pg = storages::postgres;

TEST(PostgreTopology, RttUsesExponentialWeightedAverage) {
    using namespace std::chrono_literals;

    auto rtt = pg::detail::kUnknownRtt;

    pg::detail::UpdateAverageRtt(rtt, 10ms);
    EXPECT_EQ(rtt, 10ms);

    pg::detail::UpdateAverageRtt(rtt, 20ms);
    EXPECT_EQ(rtt, 12ms);

    pg::detail::UpdateAverageRtt(rtt, 20ms);
    EXPECT_EQ(rtt, 13600us);

    pg::detail::UpdateAverageRtt(rtt, pg::detail::kUnknownRtt);
    EXPECT_EQ(rtt, 13600us);
}

USERVER_NAMESPACE_END
