#include <gtest/gtest.h>

#include <utils/statistics/percentile_format_json.hpp>

TEST(PercentileFormat, FieldName) {
  using namespace utils::statistics;

  EXPECT_EQ("p0", GetPercentileFieldName(0));
  EXPECT_EQ("p11_1", GetPercentileFieldName(11.1));
  EXPECT_EQ("p25", GetPercentileFieldName(25));
  EXPECT_EQ("p33_3", GetPercentileFieldName(33.3));
  EXPECT_EQ("p50", GetPercentileFieldName(50.0));
  EXPECT_EQ("p75", GetPercentileFieldName(75));
  EXPECT_EQ("p77_7", GetPercentileFieldName(77.7));
  EXPECT_EQ("p90", GetPercentileFieldName(90));
  EXPECT_EQ("p95", GetPercentileFieldName(95));
  EXPECT_EQ("p97_3", GetPercentileFieldName(97.3));
  EXPECT_EQ("p98", GetPercentileFieldName(98));
  EXPECT_EQ("p99", GetPercentileFieldName(99));
  EXPECT_EQ("p99_4", GetPercentileFieldName(99.4));
  EXPECT_EQ("p99_6", GetPercentileFieldName(99.6));
  EXPECT_EQ("p99_9", GetPercentileFieldName(99.9));
  EXPECT_EQ("p100", GetPercentileFieldName(100));
}
