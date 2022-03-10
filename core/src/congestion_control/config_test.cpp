#include <userver/congestion_control/config.hpp>

#include <gtest/gtest.h>

#include <userver/formats/json/serialize.hpp>

USERVER_NAMESPACE_BEGIN

TEST(CongestionControlConfig, Parsing) {
  constexpr std::string_view kPolicyJson = R"(
    {
      "min-limit": 1,
      "up-rate-percent": 2.3,
      "down-rate-percent": 4.5,
      "overload-on-seconds": 6,
      "overload-off-seconds": 7,
      "up-level": 8,
      "down-level": 9,
      "no-limit-seconds": 10,
      "load-limit-percent": 11,
      "load-limit-crit-percent": 12,
      "start-limit-factor": 13.5
    }
  )";
  const auto policy =
      formats::json::FromString(kPolicyJson).As<congestion_control::Policy>();

  EXPECT_EQ(policy.min_limit, 1);
  EXPECT_DOUBLE_EQ(policy.up_rate_percent, 2.3);
  EXPECT_DOUBLE_EQ(policy.down_rate_percent, 4.5);
  EXPECT_EQ(policy.overload_on, 6);
  EXPECT_EQ(policy.overload_off, 7);
  EXPECT_EQ(policy.up_count, 8);
  EXPECT_EQ(policy.down_count, 9);
  EXPECT_EQ(policy.no_limit_count, 10);
  EXPECT_EQ(policy.load_limit_percent, 11);
  EXPECT_EQ(policy.load_limit_crit_percent, 12);
  EXPECT_DOUBLE_EQ(policy.start_limit_factor, 13.5);
}

USERVER_NAMESPACE_END
