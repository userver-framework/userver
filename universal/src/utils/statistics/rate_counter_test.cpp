#include <userver/utils/statistics/rate_counter.hpp>

#include <gtest/gtest.h>

#include <userver/utils/statistics/rate.hpp>
#include <userver/utils/statistics/testing.hpp>

USERVER_NAMESPACE_BEGIN

namespace utils::statistics {

TEST(RateCounter, Basic) {
    {
        RateCounter test1;
        test1.Store(Rate{10});
        EXPECT_EQ(Rate{10}, test1.Load());
    }

    {
        RateCounter test1;
        test1.Store(Rate{10});
        ++test1;
        EXPECT_EQ(Rate{11}, test1.Load());
    }

    {
        RateCounter test1;
        test1.Store(Rate{10});
        test1 += Rate{10};
        EXPECT_EQ(Rate{20}, test1.Load());
    }

    {
        RateCounter test1;
        test1.Store(Rate{10});
        RateCounter test2;
        test2.Store(Rate{20});
        test1 += test2;
        EXPECT_EQ(Rate{30}, test1.Load());
    }
}

TEST(RateCounter, DumpMetric) {
    RateCounter rate_counter{Rate{10}};

    EXPECT_EQ(Snapshot{rate_counter}.SingleMetric({}), Rate{10});

    ResetMetric(rate_counter);
    EXPECT_EQ(Snapshot{rate_counter}.SingleMetric({}), Rate{0});
}

}  // namespace utils::statistics

USERVER_NAMESPACE_END
