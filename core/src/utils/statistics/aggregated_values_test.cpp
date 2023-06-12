#include <userver/utils/statistics/aggregated_values.hpp>

#include <gtest/gtest.h>

USERVER_NAMESPACE_BEGIN

TEST(AggregatedValues, Empty) {
  utils::statistics::AggregatedValues<3> av;
  EXPECT_EQ(0, av.Get(0));
  EXPECT_EQ(0, av.Get(1));
  EXPECT_EQ(0, av.Get(2));
}

TEST(AggregatedValues, SetGet) {
  utils::statistics::AggregatedValues<3> av;

  av.Add(0, 1);
  av.Add(1, 2);
  av.Add(2, 4);
  av.Add(4, 8);
  av.Add(6, 16);

  EXPECT_EQ(3, av.Get(0));
  EXPECT_EQ(4, av.Get(1));
  EXPECT_EQ(24, av.Get(2));
}

USERVER_NAMESPACE_END
