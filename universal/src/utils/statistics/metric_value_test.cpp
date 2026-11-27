#include <userver/utils/statistics/metric_value.hpp>

#include <cstdint>
#include <limits>
#include <vector>

#include <gtest/gtest.h>

#include <userver/utils/statistics/histogram.hpp>
#include <userver/utils/statistics/rate.hpp>

USERVER_NAMESPACE_BEGIN

TEST(MetricValue, EqualityDoesNotRoundIntegers) {
    constexpr auto kExactInDouble = std::int64_t{1} << 53;
    constexpr auto kNotExactInDouble = kExactInDouble + 1;
    constexpr auto kInt64Min = std::numeric_limits<std::int64_t>::min();

    const utils::statistics::MetricValue exact_int{kExactInDouble};
    const utils::statistics::MetricValue exact_double{static_cast<double>(kExactInDouble)};
    EXPECT_TRUE(exact_int.EqualsNumerically(exact_double));

    const utils::statistics::MetricValue not_exact_int{kNotExactInDouble};
    const utils::statistics::MetricValue rounded_double{static_cast<double>(kNotExactInDouble)};
    EXPECT_FALSE(not_exact_int.EqualsNumerically(rounded_double));

    const utils::statistics::MetricValue same_rate{utils::statistics::Rate{static_cast<std::uint64_t>(kNotExactInDouble)
    }};
    EXPECT_TRUE(not_exact_int.EqualsNumerically(same_rate));
    EXPECT_FALSE(same_rate.EqualsNumerically(rounded_double));
    EXPECT_FALSE(rounded_double.EqualsNumerically(same_rate));

    EXPECT_TRUE(utils::statistics::MetricValue{std::int64_t{-4}}.EqualsNumerically(-4.0));
    EXPECT_FALSE(utils::statistics::MetricValue{4.5}.EqualsNumerically(std::int64_t{4}));
    EXPECT_TRUE(utils::statistics::MetricValue{kInt64Min}.EqualsNumerically(static_cast<double>(kInt64Min)));
}

TEST(MetricValue, EqualityDoesNotTreatHistogramAsNumber) {
    const utils::statistics::Histogram histogram{std::vector<double>{1.0}};

    EXPECT_FALSE(utils::statistics::MetricValue{histogram.GetView()}.EqualsNumerically(std::int64_t{0}));
}

TEST(MetricValue, EqualitySeesHistogramBucketCounts) {
    utils::statistics::Histogram filled{std::vector<double>{1.0}};
    filled.Account(0.5);
    const utils::statistics::Histogram empty{std::vector<double>{1.0}};

    const utils::statistics::MetricValue filled_value{filled.GetView()};
    const utils::statistics::MetricValue empty_value{empty.GetView()};
    EXPECT_EQ(filled_value.AsHistogram().GetValueAt(0), 1);
    EXPECT_EQ(empty_value.AsHistogram().GetValueAt(0), 0);
    EXPECT_FALSE(filled_value.EqualsNumerically(empty_value));
}

USERVER_NAMESPACE_END
