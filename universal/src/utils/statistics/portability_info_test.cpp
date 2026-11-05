#include <userver/utils/statistics/portability_info.hpp>

#include <limits>
#include <ranges>

#include <gtest/gtest.h>
#include <userver/utils/algo.hpp>
#include <userver/utils/statistics/histogram.hpp>

#include <userver/utest/assert_macros.hpp>

USERVER_NAMESPACE_BEGIN

namespace utils::statistics {

TEST(MetricsPortabilityInfo, Inf) {
    auto warnings =
        GetPortabilityWarnings([](Writer& writer) { writer["a.inf"] = std::numeric_limits<double>::infinity(); }, {});

    ASSERT_EQ(1, warnings.size());
    const auto& [code, entries] = *warnings.begin();
    EXPECT_EQ(WarningCode::kInf, code);
    ASSERT_EQ(1, entries.size());
    EXPECT_EQ(entries[0].path, "a.inf");
}

TEST(MetricsPortabilityInfo, NanQuiet) {
    auto warnings =
        GetPortabilityWarnings([](Writer& writer) { writer["a.nan"] = std::numeric_limits<double>::quiet_NaN(); }, {});

    ASSERT_EQ(1, warnings.size());
    const auto& [code, entries] = *warnings.begin();
    EXPECT_EQ(WarningCode::kNan, code);
    ASSERT_EQ(1, entries.size());
    EXPECT_EQ(entries[0].path, "a.nan");
}

TEST(MetricsPortabilityInfo, NanSignaling) {
    auto warnings = GetPortabilityWarnings(
        [](Writer& writer) { writer["a.nan"] = std::numeric_limits<double>::signaling_NaN(); },
        {}
    );

    ASSERT_EQ(1, warnings.size());
    const auto& [code, entries] = *warnings.begin();
    EXPECT_EQ(WarningCode::kNan, code);
    ASSERT_EQ(1, entries.size());
    EXPECT_EQ(entries[0].path, "a.nan");
}

TEST(MetricsPortabilityInfo, TooManyLabels) {
    auto warnings = GetPortabilityWarnings(
        [](Writer& writer) {
            writer["a.labels"].ValueWithLabels(
                42,
                {
                    {"0", "0"},   {"1", "1"},   {"2", "2"},   {"3", "3"},   {"4", "4"},   {"5", "5"},   {"6", "6"},
                    {"7", "7"},   {"8", "8"},   {"9", "9"},   {"10", "10"}, {"11", "11"}, {"12", "12"}, {"13", "13"},
                    {"14", "14"}, {"15", "15"}, {"16", "16"}, {"17", "17"}, {"18", "18"}, {"19", "19"}, {"20", "20"},
                    {"21", "21"}, {"22", "22"}, {"23", "23"}, {"24", "24"}, {"25", "25"},
                }
            );
        },
        {}
    );

    ASSERT_EQ(1, warnings.size());
    const auto& [code, entries] = *warnings.begin();
    EXPECT_EQ(WarningCode::kLabelsCount, code);
    ASSERT_EQ(1, entries.size());
    EXPECT_EQ(entries[0].path, "a.labels");
}

struct NameCode {
    std::string_view name;
    WarningCode code;
};

class PortabilityInfoLabels : public testing::TestWithParam<NameCode> {};

TEST_P(PortabilityInfoLabels, ReservedLabels) {
    auto warnings = GetPortabilityWarnings(
        [](Writer& writer) { writer["a.label"].ValueWithLabels(42, {GetParam().name, "1"}); },
        {}
    );

    ASSERT_EQ(1, warnings.size());
    const auto& [code, entries] = *warnings.begin();
    EXPECT_EQ(GetParam().code, code);
    ASSERT_EQ(1, entries.size());
    EXPECT_EQ(entries[0].path, "a.label");
}

INSTANTIATE_TEST_SUITE_P(
    Metrics,
    PortabilityInfoLabels,
    ::testing::Values(
        NameCode{"application", WarningCode::kReservedLabelApplication},
        NameCode{"project", WarningCode::kReservedLabelProject},
        NameCode{"cluster", WarningCode::kReservedLabelCluster},
        NameCode{"service", WarningCode::kReservedLabelService},
        NameCode{"host", WarningCode::kReservedLabelHost},
        NameCode{"group", WarningCode::kReservedLabelGroup},
        NameCode{"sensor", WarningCode::kReservedLabelSensor}
    ),
    [](const auto& warnings) { return std::string{warnings.param.name}; }
);

TEST(MetricsPortabilityInfo, LabelNameLong) {
    auto warnings = GetPortabilityWarnings(
        [](Writer& writer) {
            std::string s;
            s.resize(201, '!');
            writer["a.name"].ValueWithLabels(42, {s, "1"});
        },
        {}
    );

    ASSERT_EQ(1, warnings.size());
    const auto& [code, entries] = *warnings.begin();
    EXPECT_EQ(WarningCode::kLabelNameLength, code);
    ASSERT_EQ(1, entries.size());
    EXPECT_EQ(entries[0].path, "a.name");
}

TEST(MetricsPortabilityInfo, LabelValueLong) {
    auto warnings = GetPortabilityWarnings(
        [](Writer& writer) {
            std::string s;
            s.resize(201, '!');
            writer["a.value"].ValueWithLabels(42, {"1", s});
        },
        {}
    );

    ASSERT_EQ(1, warnings.size());
    const auto& [code, entries] = *warnings.begin();
    EXPECT_EQ(WarningCode::kLabelValueLength, code);
    ASSERT_EQ(1, entries.size());
    EXPECT_EQ(entries[0].path, "a.value");
}

TEST(MetricsPortabilityInfo, PathLong) {
    auto warnings = GetPortabilityWarnings(
        [](Writer& writer) {
            std::string s;
            s.resize(201, '!');
            writer["a.value"][s] = 42;
        },
        {}
    );

    ASSERT_EQ(1, warnings.size());
    const auto& [code, entries] = *warnings.begin();
    EXPECT_EQ(WarningCode::kPathLength, code);
    ASSERT_EQ(1, entries.size());
    EXPECT_EQ(entries[0].path.substr(0, 7), "a.value");
}

TEST(MetricsPortabilityInfo, LabelsMismatch) {
    auto warnings = GetPortabilityWarnings(
        [](Writer& writer) {
            writer["a.mismatch"].ValueWithLabels(42, {"a", "1"});
            writer["a.mismatch"].ValueWithLabels(42, {"b", "1"});
        },
        {}
    );

    ASSERT_EQ(1, warnings.size());
    const auto& [code, entries] = *warnings.begin();
    EXPECT_EQ(WarningCode::kLabelNameMismatch, code);
    ASSERT_EQ(1, entries.size());
    EXPECT_EQ(entries[0].path, "a.mismatch");
}

TEST(MetricsPortabilityInfo, HistogramBucketsCount) {
    auto warnings = GetPortabilityWarnings(
        [](Writer& writer) {
            writer["test"] = utils::statistics::Histogram{
                utils::impl::AsContainerViaInsert<std::vector<double>>(std::views::iota(1, 300))
            };
        },
        {}
    );

    ASSERT_EQ(1, warnings.size());
    const auto& [code, entries] = *warnings.begin();
    EXPECT_EQ(WarningCode::kHistogramBucketsCount, code);
    ASSERT_EQ(1, entries.size());
    EXPECT_EQ(entries[0].path, "test");
}

}  // namespace utils::statistics

USERVER_NAMESPACE_END
