#include <userver/utils/statistics/rate_counter.hpp>

#include <userver/utest/utest.hpp>
#include <userver/utils/statistics/rate.hpp>

USERVER_NAMESPACE_BEGIN

namespace utils::statistics {

UTEST(RateCounter, Basic) {
  {
    RateCounter test1;
    test1.Store(Rate{10});
    EXPECT_EQ(Rate{10}, test1.Load());
  }

  {
    RateCounter test1;
    test1.Store(Rate{10});
    test1++;
    EXPECT_EQ(Rate{11}, test1.Load());
  }

  {
    RateCounter test1;
    test1.Store(Rate{10});
    test1 += Rate{10};
    EXPECT_EQ(Rate{20}, test1.Load());
  }
}

}  // namespace utils::statistics

USERVER_NAMESPACE_END
