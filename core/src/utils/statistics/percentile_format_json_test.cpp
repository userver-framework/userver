#include <gtest/gtest.h>

#include <userver/formats/json/serialize.hpp>
#include <userver/utils/statistics/percentile_format_json.hpp>

USERVER_NAMESPACE_BEGIN

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

TEST(PercentileFormat, GetPercentileMethod) {
  const auto expected =
      formats::json::FromString(R"({"p99":1,"p99_9":1,"p100":1,
          "$meta":{"solomon_children_labels":"percentile"}})");

  struct CaseHasGoodMethod final {
    size_t GetPercentile(double) const { return 1; }
  };

  const auto value =
      utils::statistics::PercentileToJson(CaseHasGoodMethod(), {99, 99.9, 100})
          .ExtractValue();
  EXPECT_EQ(value, expected);

  // This should not compile
  // struct CaseHasNoMethod {};
  // EXPECT_EQ(utils::statistics::PercentileToJson(
  //          CaseHasNoMethod(), {99, 99.9, 100}).ExtractValue(),
  //          expected);

  // This should not compile
  // struct CasePropInsteadMethod {
  //  int GetPercentile = 1;
  // };
  // EXPECT_EQ(utils::statistics::PercentileToJson(
  //           CasePropInsteadMethod(), {99, 99.9, 100}).ExtractValue(),
  //           expected);
}

USERVER_NAMESPACE_END
