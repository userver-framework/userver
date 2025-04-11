#include <userver/utils/statistics/pretty_format.hpp>

#include <userver/utest/utest.hpp>
#include <userver/utils/statistics/storage.hpp>

USERVER_NAMESPACE_BEGIN

namespace {

int HeavyComputation() { return 4; }

int HeavyComputation2() { return 5; }

UTEST(MetricsPrettyFormat, SampleBasic) {
    utils::statistics::Storage storage;

    utils::statistics::Entry holder_;
    /// [Writer basic sample]
    // `storage` is a utils::statistics::Storage, `holder_` is a component member of type utils::statistics::Entry
    holder_ = storage.RegisterWriter(
        "path-begin",
        [&](utils::statistics::Writer& writer) {
            // Metric without labels
            writer["metric1"] = 1;

            // Metric with single label
            writer["metric2"].ValueWithLabels(2, {"label_name1", "value1"});

            // Metric with multiple labels
            writer["metric3"].ValueWithLabels(3, {{"label_name2", "value2"}, {"label_name3", "value3"}});

            // Compute and write a bunch of heavy metrics only if they were requested
            if (auto subpath = writer["a"]["b"]) {
                subpath["mc4"] = HeavyComputation();
                subpath["mc5"] = HeavyComputation2();
            }
        },
        {{"label_for_all", "value_all"}}
    );
    /// [Writer basic sample]

    auto result = utils::statistics::ToPrettyFormat(storage, utils::statistics::Request::MakeWithPrefix("path-begin"));

    constexpr std::string_view ethalon = /** [metrics pretty] */ R"(
path-begin.metric1: label_for_all=value_all	GAUGE	1
path-begin.metric2: label_for_all=value_all, label_name1=value1	GAUGE	2
path-begin.metric3: label_for_all=value_all, label_name2=value2, label_name3=value3	GAUGE	3
path-begin.a.b.mc4: label_for_all=value_all	GAUGE	4
path-begin.a.b.mc5: label_for_all=value_all	GAUGE	5
)"; /** [metrics pretty] */

    EXPECT_EQ(result, ethalon.substr(1));
}

UTEST(MetricsPrettyFormat, CheckFormat) {
    utils::statistics::Storage storage;

    const auto entry = storage.RegisterWriter(
        "best_prefix",
        [](utils::statistics::Writer& writer) {
            writer.ValueWithLabels(42, {{"label_1", "value_1"}, {"label_2", "value_2"}, {"label_3", "value_3"}});
        },
        {}
    );

    const auto request = utils::statistics::Request::MakeWithPrefix("best_prefix");
    constexpr auto expected =
        "best_prefix: label_1=value_1, label_2=value_2, "
        "label_3=value_3\tGAUGE\t42\n";
    const std::string result_str = utils::statistics::ToPrettyFormat(storage, request);
    EXPECT_EQ(result_str, expected);
}

UTEST(MetricsPrettyFormat, CheckSortingInFormat) {
    utils::statistics::Storage storage;

    const auto entry = storage.RegisterWriter(
        "best_prefix",
        [](utils::statistics::Writer& writer) {
            writer.ValueWithLabels(42, {{"label_3", "value_3"}, {"label_2", "value_2"}, {"label_1", "value_1"}});
        },
        {}
    );

    const auto request = utils::statistics::Request::MakeWithPrefix("best_prefix");
    constexpr auto expected =
        "best_prefix: label_1=value_1, label_2=value_2, "
        "label_3=value_3\tGAUGE\t42\n";
    const std::string result_str = utils::statistics::ToPrettyFormat(storage, request);
    EXPECT_EQ(result_str, expected);
}

UTEST(MetricsPrettyFormat, NoLabels) {
    utils::statistics::Storage storage;

    const auto entry = storage.RegisterWriter(
        "best_prefix", [](utils::statistics::Writer& writer) { writer.ValueWithLabels(42, {}); }, {}
    );

    const auto request = utils::statistics::Request::MakeWithPrefix("best_prefix");
    constexpr auto expected = "best_prefix:\tGAUGE\t42\n";
    const std::string result_str = utils::statistics::ToPrettyFormat(storage, request);
    EXPECT_EQ(result_str, expected);
}

UTEST(MetricsPrettyFormat, Rate) {
    utils::statistics::Storage storage;

    const auto entry = storage.RegisterWriter(
        "best_prefix", [](utils::statistics::Writer& writer) { writer = utils::statistics::Rate{42}; }, {}
    );

    const auto request = utils::statistics::Request::MakeWithPrefix("best_prefix");
    constexpr auto expected = "best_prefix:\tRATE\t42\n";
    const std::string result_str = utils::statistics::ToPrettyFormat(storage, request);
    EXPECT_EQ(result_str, expected);
}

}  // namespace

USERVER_NAMESPACE_END
