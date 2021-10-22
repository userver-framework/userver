#include <userver/utils/mock_now.hpp>
#include <userver/utils/statistics/percentile.hpp>

#include <gtest/gtest.h>

USERVER_NAMESPACE_BEGIN

TEST(Percentile, Zero) {
  utils::statistics::Percentile<100> p;

  EXPECT_EQ(0u, p.GetPercentile(0));
  EXPECT_EQ(0u, p.GetPercentile(50));
  EXPECT_EQ(0u, p.GetPercentile(100));
}

TEST(Percentile, One) {
  utils::statistics::Percentile<100> p;

  p.Account(3);

  EXPECT_EQ(3u, p.GetPercentile(0));
  EXPECT_EQ(3u, p.GetPercentile(50));
  EXPECT_EQ(3u, p.GetPercentile(100));
}

TEST(Percentile, Hundred) {
  utils::statistics::Percentile<100> p;

  for (int i = 0; i < 100; i++) p.Account(i);

  EXPECT_EQ(0u, p.GetPercentile(0));
  EXPECT_EQ(50u, p.GetPercentile(50));
  EXPECT_EQ(99u, p.GetPercentile(100));
  EXPECT_EQ(99u, p.GetPercentile(101));
  EXPECT_EQ(99u, p.GetPercentile(200));
}

TEST(Percentile, Many) {
  utils::statistics::Percentile<3> p;

  p.Account(0);
  p.Account(0);
  p.Account(0);
  p.Account(1);
  p.Account(1);

  EXPECT_EQ(0u, p.GetPercentile(0));
  EXPECT_EQ(0u, p.GetPercentile(50));
  EXPECT_EQ(1u, p.GetPercentile(100));
  EXPECT_EQ(1u, p.GetPercentile(101));
  EXPECT_EQ(1u, p.GetPercentile(200));
}

TEST(Percentile, Extra100perc) {
  utils::statistics::Percentile<10, unsigned int, 10, 10> p;

  p.Account(0);
  EXPECT_EQ(0u, p.GetPercentile(100));

  p.Account(7);
  EXPECT_EQ(7u, p.GetPercentile(100));

  p.Account(9);
  EXPECT_EQ(9u, p.GetPercentile(100));

  p.Account(10);
  EXPECT_EQ(10u, p.GetPercentile(100));

  p.Account(13);
  EXPECT_EQ(10u, p.GetPercentile(100));

  p.Account(19);
  EXPECT_EQ(20u, p.GetPercentile(100));

  p.Account(21);
  EXPECT_EQ(20u, p.GetPercentile(100));

  p.Account(100);
  EXPECT_EQ(100u, p.GetPercentile(100));

  p.Account(101);
  EXPECT_EQ(100u, p.GetPercentile(100));

  p.Account(201);
  EXPECT_EQ(100u, p.GetPercentile(100));
}

USERVER_NAMESPACE_END
