#include <userver/utils/statistics/pretty_format.hpp>

#include <gtest/gtest.h>

#include <userver/utest/assert_macros.hpp>
#include <userver/utils/statistics/rate.hpp>

USERVER_NAMESPACE_BEGIN

namespace {

using utils::statistics::Request;
using utils::statistics::Writer;

TEST(MetricsPrettyFormat, SampleBasic) {
    const std::string result = utils::statistics::ToPrettyFormat([](Writer& writer) {
        writer["metric1"] = 1;
        writer["metric2"].ValueWithLabels(2, {"label_name1", "value1"});
        writer["metric3"].ValueWithLabels(3, {{"label_name2", "value2"}, {"label_name3", "value3"}});
    });
    constexpr std::string_view expected =
        "metric1:\tGAUGE\t1\n"
        "metric2: label_name1=value1\tGAUGE\t2\n"
        "metric3: label_name2=value2, label_name3=value3\tGAUGE\t3\n";
    EXPECT_EQ(result, expected);
}

TEST(MetricsPrettyFormat, CheckFormat) {
    constexpr auto expected =
        "best_prefix: label_1=value_1, label_2=value_2, "
        "label_3=value_3\tGAUGE\t42\n";
    const std::string result_str = utils::statistics::ToPrettyFormat([](Writer& writer) {
        writer["best_prefix"]
            .ValueWithLabels(42, {{"label_1", "value_1"}, {"label_2", "value_2"}, {"label_3", "value_3"}});
    });
    EXPECT_EQ(result_str, expected);
}

TEST(MetricsPrettyFormat, CheckSortingInFormat) {
    constexpr auto expected =
        "best_prefix: label_1=value_1, label_2=value_2, "
        "label_3=value_3\tGAUGE\t42\n";
    const std::string result_str = utils::statistics::ToPrettyFormat([](Writer& writer) {
        writer["best_prefix"]
            .ValueWithLabels(42, {{"label_3", "value_3"}, {"label_2", "value_2"}, {"label_1", "value_1"}});
    });
    EXPECT_EQ(result_str, expected);
}

TEST(MetricsPrettyFormat, NoLabels) {
    constexpr auto expected = "best_prefix:\tGAUGE\t42\n";
    const std::string result_str = utils::statistics::ToPrettyFormat([](Writer& writer) {
        writer["best_prefix"].ValueWithLabels(42, {});
    });
    EXPECT_EQ(result_str, expected);
}

TEST(MetricsPrettyFormat, Rate) {
    constexpr auto expected = "best_prefix:\tRATE\t42\n";
    const std::string result_str = utils::statistics::ToPrettyFormat([](Writer& writer) {
        writer["best_prefix"] = utils::statistics::Rate{42};
    });
    EXPECT_EQ(result_str, expected);
}

}  // namespace

USERVER_NAMESPACE_END
