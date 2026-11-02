#include <userver/utils/statistics/storage.hpp>

#include <userver/formats/json/serialize.hpp>

#include <memory>
#include <userver/rcu/rcu_map.hpp>
#include <userver/utest/utest.hpp>
#include <userver/utils/mock_now.hpp>
#include <userver/utils/resource_scopes.hpp>
#include <userver/utils/statistics/graphite.hpp>
#include <userver/utils/statistics/json.hpp>
#include <userver/utils/statistics/pretty_format.hpp>
#include <userver/utils/statistics/prometheus.hpp>

using USERVER_NAMESPACE::utils::statistics::Request;
using USERVER_NAMESPACE::utils::statistics::Storage;
using USERVER_NAMESPACE::utils::statistics::ToPrometheusFormatUntyped;
using USERVER_NAMESPACE::utils::statistics::Writer;

USERVER_NAMESPACE_BEGIN

namespace {

using utils::statistics::Request;
using utils::statistics::Storage;
using utils::statistics::ToPrometheusFormatUntyped;
using utils::statistics::Writer;

using MetricLabels = std::vector<utils::statistics::Label>;

struct SelfContainedMetricLambda final {
    SelfContainedMetricLambda(
        utils::ResourceScopeStorage& scopes,
        Storage& storage,
        std::string prefix,
        MetricLabels labels,
        utils::statistics::WriterFunc writer
    )
    {
        storage.RegisterWriter(scopes, std::move(prefix), std::move(writer), std::move(labels));
    }
};

std::shared_ptr<SelfContainedMetricLambda> MakeSelfContainedMetricLambda(
    Storage& storage,
    std::string prefix,
    MetricLabels labels,
    utils::statistics::WriterFunc writer
) {
    return utils::MakeWithResourceScopes<
        SelfContainedMetricLambda>(storage, std::move(prefix), std::move(labels), std::move(writer));
}

struct ComponentMetricsNested {
    std::atomic<int> nested1{17};
    std::atomic<int> nested2{18};
};

[[maybe_unused]] void DumpMetric(utils::statistics::Writer& writer, const ComponentMetricsNested& m) {
    writer["nested1"] = m.nested1;
    writer["nested2"].ValueWithLabels(m.nested2, {"label_name", "label_value"});
}

struct ComponentMetrics {
    rcu::RcuMap<std::string, ComponentMetricsNested> metrics;
};

[[maybe_unused]] void DumpMetric(utils::statistics::Writer& writer, const ComponentMetrics& m) {
    for (const auto& [key, value] : m.metrics) {
        UASSERT(value);
        writer.ValueWithLabels(*value, {{"a_label", key}, {"b_label", "b"}});
    }
}

int HeavyComputation() { return 4; }

int HeavyComputation2() { return 5; }

class FooBarMetrics {
public:
    FooBarMetrics(utils::ResourceScopeStorage& scopes, Storage& storage) {
        storage.RegisterWriter(scopes, "foo.bar.baz", [](utils::statistics::Writer& writer) { writer = 42; });
    }
};

class SampleMetricsHolder {
public:
    SampleMetricsHolder(utils::ResourceScopeStorage& scopes, Storage& storage) {
        /// [Writer basic sample]
        // `storage` is utils::statistics::Storage; `scopes` is utils::ResourceScopeStorage that owns the writer
        // lifetime.
        storage.RegisterWriter(
            scopes,
            "path-begin",
            [&](utils::statistics::Writer& writer) {
                writer["metric1"] = 1;
                writer["metric2"].ValueWithLabels(2, {"label_name1", "value1"});
                writer["metric3"].ValueWithLabels(3, {{"label_name2", "value2"}, {"label_name3", "value3"}});
                if (auto subpath = writer["a"]["b"]) {
                    subpath["mc4"] = HeavyComputation();
                    subpath["mc5"] = HeavyComputation2();
                }
            },
            {{"label_for_all", "value_all"}}
        );
        /// [Writer basic sample]
    }
};

struct MetricTypeThatMayBeDumped {};
[[maybe_unused]] void DumpMetric(Writer& writer, MetricTypeThatMayBeDumped) { writer = 42; }

struct MetricTypeThatMustBeSkipped {
    const char* context;
};
[[maybe_unused]] void DumpMetric(Writer&, MetricTypeThatMustBeSkipped metric) {
    ADD_FAILURE() << "Metric dumping was called " << metric.context;
}

void DoTestBasic(Storage& storage) {
    const auto* const expected = "prefix_name{} 42\n";
    EXPECT_EQ(expected, ToPrometheusFormatUntyped(storage));
    EXPECT_EQ(expected, ToPrometheusFormatUntyped(storage, Request::MakeWithPrefix("prefix")));
    EXPECT_EQ(expected, ToPrometheusFormatUntyped(storage, Request::MakeWithPrefix("pre")));

    const auto extra_prefix = MakeSelfContainedMetricLambda(storage, "prefix", {}, [](Writer& writer) { writer = 41; });
    const auto extra_prefix_name = MakeSelfContainedMetricLambda(storage, "prEfix.name", {}, [](Writer& writer) {
        writer = 43;
    });
    const auto extra_a = MakeSelfContainedMetricLambda(storage, std::string{}, {}, [](Writer& writer) {
        writer["a"] = 44;
    });

    EXPECT_EQ(expected, ToPrometheusFormatUntyped(storage, Request::MakeWithPrefix("prefix.")));
    EXPECT_EQ(expected, ToPrometheusFormatUntyped(storage, Request::MakeWithPrefix("prefix.name")));
    EXPECT_EQ(expected, ToPrometheusFormatUntyped(storage, Request::MakeWithPath("prefix.name")));

    const auto* const expected_labeled = "prefix_name{lab=\"foo\"} 42\n";
    EXPECT_EQ(
        expected_labeled,
        ToPrometheusFormatUntyped(storage, Request::MakeWithPrefix("prefix.name", {{"lab", "foo"}}))
    );

    EXPECT_EQ(
        expected_labeled,
        ToPrometheusFormatUntyped(storage, Request::MakeWithPrefix("prefix.name", {{"lab", "foo"}}, {{"lab", "foo"}}))
    );
    const auto* const expected_all = R"(
prefix_name{} 42
prefix{} 41
prEfix_name{} 43
a{} 44
)";
    EXPECT_EQ(expected_all, "\n" + ToPrometheusFormatUntyped(storage, Request{}));
}

namespace some {

struct Dumpable1 {};
[[maybe_unused]] void DumpMetric(Writer&, const Dumpable1&) {}

struct Dumpable2 {};
[[maybe_unused]] void DumpMetric(Writer&, Dumpable2) {}

struct Dumpable3 {};

}  // namespace some

}  // namespace

namespace utils::statistics {
[[maybe_unused]] static void DumpMetric(Writer&, some::Dumpable3) {}
}  // namespace utils::statistics

UTEST(StatisticsStorage, RegisterWriter) {
    Storage statistics_storage;
    const utils::WithResourceScopes<FooBarMetrics> metrics(std::in_place, statistics_storage);

    const auto json = formats::json::FromString(utils::statistics::ToJsonFormat(statistics_storage));
    EXPECT_EQ(json["foo.bar.baz"][0]["value"].As<int>(), 42);
}

UTEST(StatisticsStorage, PrettyFormatWriterSample) {
    Storage storage;
    const utils::WithResourceScopes<SampleMetricsHolder> metrics(std::in_place, storage);

    const auto
        result = utils::statistics::ToPrettyFormat(storage, utils::statistics::Request::MakeWithPrefix("path-begin"));

    constexpr std::string_view ethalon = /** [metrics pretty] */
        ("path-begin.metric1: label_for_all=value_all\tGAUGE\t1\n"
         "path-begin.metric2: label_for_all=value_all, label_name1=value1\tGAUGE\t2\n"
         "path-begin.metric3: label_for_all=value_all, label_name2=value2, label_name3=value3\tGAUGE\t3\n"
         "path-begin.a.b.mc4: label_for_all=value_all\tGAUGE\t4\n"
         "path-begin.a.b.mc5: label_for_all=value_all\tGAUGE\t5\n"); /** [metrics pretty] */

    EXPECT_EQ(result, ethalon);
}

UTEST(StatisticsStorage, UnsubscribingCheckWriterData) {
    Storage storage;
    int counter = 0;
    const auto writer_func = [&counter](utils::statistics::Writer& writer) {
        writer = 1;
        writer["a"] = 2;
        counter++;
    };

    auto holder1 = storage.RegisterWriter("prefix1", writer_func);
    {
        auto holder2 = storage.RegisterWriter("prefix2", writer_func);
        holder2.Unregister();
    }
    {
        auto holder3 = storage.RegisterWriter("prefix3", writer_func);
    }

    if constexpr (utils::statistics::impl::kCheckSubscriptionUB) {
        EXPECT_EQ(counter, 5);
    } else {
        EXPECT_EQ(counter, 0);
    }
}

UTEST(MetricsWriter, Basic1) {
    Storage storage;
    const auto metrics = MakeSelfContainedMetricLambda(storage, "prefix.name", {}, [](Writer& writer) { writer = 42; });
    DoTestBasic(storage);
}

UTEST(MetricsWriter, Basic2) {
    Storage storage;
    const auto metrics = MakeSelfContainedMetricLambda(storage, "", {}, [](Writer& writer) {
        writer["prefix.name"] = 42;
    });
    DoTestBasic(storage);
}

UTEST(MetricsWriter, Basic3) {
    Storage storage;
    const auto metrics = MakeSelfContainedMetricLambda(storage, "prefix", {}, [](Writer& writer) {
        writer["name"] = 42;
        writer["a"] = some::Dumpable1{};
        writer["b"] = some::Dumpable2{};
        writer["c"] = some::Dumpable3{};
    });
    DoTestBasic(storage);
}

UTEST(MetricsWriter, Sample) {
    ComponentMetrics component_metrics;
    component_metrics.metrics.Insert("metric-in-map", std::make_shared<ComponentMetricsNested>());

    Storage storage;
    constexpr std::chrono::seconds kDuration{1674810371};
    utils::datetime::MockNowSet(std::chrono::system_clock::time_point{kDuration});

    /// [DumpMetric RegisterWriter]
    const auto metrics = MakeSelfContainedMetricLambda(
        storage,
        "begin-on-the-metric-path",
        std::vector<utils::statistics::Label>{{"label_for_all", "labell value"}},
        [&](Writer& writer) {
            writer["metric1"] = 42;
            const ComponentMetrics& metrics_ref = component_metrics;
            writer["more_metrics"] = metrics_ref;
        }
    );
    /// [DumpMetric RegisterWriter]

    const auto result = utils::statistics::ToGraphiteFormat(storage, Request::MakeWithPrefix("begin"));

    constexpr std::string_view ethalon = /** [metrics graphite] */ R"(
begin-on-the-metric-path.metric1;label_for_all=labell_value 42 1674810371
begin-on-the-metric-path.more_metrics.nested1;label_for_all=labell_value;a_label=metric-in-map;b_label=b 17 1674810371
begin-on-the-metric-path.more_metrics.nested2;label_for_all=labell_value;a_label=metric-in-map;b_label=b;label_name=label_value 18 1674810371
)"; /** [metrics graphite] */

    EXPECT_EQ(result, ethalon.substr(1));
}

UTEST(MetricsWriter, WithError) {
#ifndef NDEBUG
    GTEST_SKIP() << "Writer exception handling is verified only in release builds";
#endif

    Storage storage;
    const auto first = MakeSelfContainedMetricLambda(storage, "prefix", {}, [](Writer& writer) {
        writer["name"] = 42;
        throw std::runtime_error("Something went wrong");
    });
    DoTestBasic(storage);

    const auto w_otherprefix = MakeSelfContainedMetricLambda(storage, "otherprefix", {}, [](Writer& writer) {
        writer["foo"] = 41;
        std::string too_long_path(70000, '!');
        writer[too_long_path] = 411;
    });
    const auto w_a_b = MakeSelfContainedMetricLambda(storage, "a", {}, [](Writer& writer) {
        writer["b"] = 4;
        std::string too_long_path(70000, '?');
        writer[too_long_path].ValueWithLabels(412.0, {"hello", "world"});
    });
    const auto w_x = MakeSelfContainedMetricLambda(storage, "x", {}, [](Writer& writer) {
        writer["a"] = 14;
        auto b = writer["b"];
        auto c = writer["c"];
        b = 1.0;
        c = std::size_t{2};
    });
    const std::string too_long_path(70000, 'a');
    const auto w_long_prefix = MakeSelfContainedMetricLambda(storage, too_long_path, {}, [](Writer& writer) {
        writer["q"] = 24;
    });
    const auto w_some_path = MakeSelfContainedMetricLambda(storage, std::string{}, {}, [](Writer& writer) {
        writer["some"]["path"].ValueWithLabels(3, {"name", "value"});
        writer["some"]["path"].ValueWithLabels(
            [](bool raise) { return raise ? throw std::runtime_error{"Oops"} : 1; }(true),
            {"name", "value"}
        );
    });
    const auto w_some_a = MakeSelfContainedMetricLambda(
        storage,
        "some",
        std::vector<utils::statistics::Label>{{"name2", "value2"}},
        [](Writer& writer) {
            writer["a"].ValueWithLabels(7, {});
            auto b = writer["b"];
            auto c = writer["c"];
            b = 1UL;
        }
    );
    const auto w_some_b = MakeSelfContainedMetricLambda(
        storage,
        "some",
        std::vector<utils::statistics::Label>{{"name", "value"}},
        [](Writer& writer) { writer["b"].ValueWithLabels(8, {"name2", "value2"}); }
    );
    const auto w_some_path_1 = MakeSelfContainedMetricLambda(storage, std::string{}, {}, [](Writer& writer) {
        writer["some"]["path"]["1"] = 100500;
        writer = 0;
    });
    const auto w_some_path_2 = MakeSelfContainedMetricLambda(storage, "some", {}, [](Writer& writer) {
        writer["path"]["2"] = 100502;
        writer[""] = 10;
    });
    const auto w_some_path_3 = MakeSelfContainedMetricLambda(storage, "some", {}, [](Writer& writer) {
        writer["path"]["3"] = 100503;
    });
    const auto w_some_path_4 = MakeSelfContainedMetricLambda(storage, "some", {}, [](Writer& writer) {
        writer["path"]["4"] = 100504;
    });
    const auto w_some_path_5 = MakeSelfContainedMetricLambda(storage, "some", {}, [](Writer& writer) {
        writer["path"]["5"] = 100505;
    });
    const auto w_fine = MakeSelfContainedMetricLambda(storage, "fine", {}, [](Writer& writer) { writer["ok"] = 1; });

    const auto* const expected = R"(
prefix_name{} 42
otherprefix_foo{} 41
a_b{} 4
x_a{} 14
some_path{name="value"} 3
some_a{name2="value2"} 7
some_b{name="value",name2="value2"} 8
some_path_1{} 100500
some_path_2{} 100502
some_path_3{} 100503
some_path_4{} 100504
some_path_5{} 100505
fine_ok{} 1
)";
    EXPECT_EQ(expected, "\n" + ToPrometheusFormatUntyped(storage));
}

UTEST(MetricsWriter, LabeledSingle) {
    Storage storage;
    const auto w_prefix_name = MakeSelfContainedMetricLambda(storage, "prefix", {}, [](Writer& writer) {
        writer["name"].ValueWithLabels(42, {"label_name", "label_value1"});
        writer["name"].ValueWithLabels(43, {"label_name", "label_value2"});
    });
    const auto w_prefix_values = MakeSelfContainedMetricLambda(storage, "prefix", {}, [](Writer& writer) {
        writer.ValueWithLabels(52, {"label_name", "label_value1"});
        writer.ValueWithLabels(53, {"label_name", "label_value2"});
    });
    const auto w_prefix_name_direct = MakeSelfContainedMetricLambda(storage, "prEfix.name", {}, [](Writer& writer) {
        writer.ValueWithLabels(62, {"label_name", "label_value1"});
        writer.ValueWithLabels(63, {"label_name", "label_value2"});
    });

    const auto* const expected_labeled =
        "prefix_name{label_name=\"label_value1\"} 42\n"
        "prefix_name{label_name=\"label_value2\"} 43\n";
    EXPECT_EQ(expected_labeled, ToPrometheusFormatUntyped(storage, Request::MakeWithPath("prefix.name")));

    const auto* const expected_labeled2 = "prefix_name{label_name=\"label_value2\"} 43\n";
    EXPECT_EQ(
        expected_labeled2,
        ToPrometheusFormatUntyped(storage, Request::MakeWithPath("prefix.name", {}, {{"label_name", "label_value2"}}))
    );
}

UTEST(MetricsWriter, LabeledMultiple) {
    Storage storage;
    const utils::statistics::LabelView label{"name", "value"};
    const auto w_a_1 = MakeSelfContainedMetricLambda(storage, "a", {}, [label](Writer& writer) {
        writer["1"].ValueWithLabels(1, {label, {"label_name", "label_value1"}});
        writer["1"].ValueWithLabels(2, {label, {"label_name", "label_value2"}});
    });
    const auto w_a_values = MakeSelfContainedMetricLambda(storage, "a", {}, [label](Writer& writer) {
        writer.ValueWithLabels(52, {label, {"label_name", "label_value1"}});
        writer.ValueWithLabels(53, {label, {"label_name", "label_value2"}});
    });
    const auto w_b = MakeSelfContainedMetricLambda(storage, "b", {}, [label](Writer& writer) {
        writer.ValueWithLabels(62, {label, {"label_name", "label_value1"}});
        writer.ValueWithLabels(63, {label, {"label_name", "label_value2"}});
        writer.ValueWithLabels(63, {label, {"label_name", "label_value3"}});
    });
    const auto w_c = MakeSelfContainedMetricLambda(
        storage,
        "c",
        std::vector<utils::statistics::Label>{{"name", "value"}},
        [](Writer& writer) {
            writer.ValueWithLabels(62, {{"label_name", "label_value1"}});
            writer.ValueWithLabels(63, {"label_name", "label_value2"});
            writer.ValueWithLabels(63, {{"label_name", "label_value3"}});
        }
    );

    const auto* const expected_labeled =
        "a_1{name=\"value\",label_name=\"label_value1\"} 1\n"
        "a_1{name=\"value\",label_name=\"label_value2\"} 2\n";
    EXPECT_EQ(expected_labeled, ToPrometheusFormatUntyped(storage, Request::MakeWithPath("a.1")));

    const auto* const expected_labeled2 = "a_1{name=\"value\",label_name=\"label_value2\"} 2\n";
    EXPECT_EQ(
        expected_labeled2,
        ToPrometheusFormatUntyped(storage, Request::MakeWithPath("a.1", {}, {{"label_name", "label_value2"}}))
    );
    EXPECT_EQ(
        expected_labeled2,
        ToPrometheusFormatUntyped(
            storage,
            Request::MakeWithPath("a.1", {}, {{"name", "value"}, {"label_name", "label_value2"}})
        )
    );

    const auto* const expected_labeled2_a = "a{name=\"value\",label_name=\"label_value2\"} 53\n";
    EXPECT_EQ(
        expected_labeled2_a,
        ToPrometheusFormatUntyped(storage, Request::MakeWithPath("a", {}, {{"label_name", "label_value2"}}))
    );
    EXPECT_EQ(
        expected_labeled2_a,
        ToPrometheusFormatUntyped(
            storage,
            Request::MakeWithPath("a", {}, {{"name", "value"}, {"label_name", "label_value2"}})
        )
    );

    const auto* const expected_labeled2_c = "c{name=\"value\",label_name=\"label_value2\"} 63\n";
    EXPECT_EQ(
        expected_labeled2_c,
        ToPrometheusFormatUntyped(storage, Request::MakeWithPath("c", {}, {{"label_name", "label_value2"}}))
    );
    EXPECT_EQ(
        expected_labeled2_c,
        ToPrometheusFormatUntyped(
            storage,
            Request::MakeWithPath("c", {}, {{"name", "value"}, {"label_name", "label_value2"}})
        )
    );
}

UTEST(MetricsWriter, CustomTypesOptimization) {
    if constexpr (utils::statistics::impl::kCheckSubscriptionUB) {
        return;
    }
    Storage storage;
    const auto w_skip = MakeSelfContainedMetricLambda(storage, "skip", {}, [](Writer& writer) {
        writer = MetricTypeThatMustBeSkipped{"at 'skip' path"};
    });
    const auto w_a_skip = MakeSelfContainedMetricLambda(storage, "a", {}, [](Writer& writer) {
        writer["skip"] = MetricTypeThatMustBeSkipped{"at 'a.skip' path"};
    });
    const auto w_a_dump = MakeSelfContainedMetricLambda(storage, "a", {}, [](Writer& writer) {
        writer["dump"] = MetricTypeThatMayBeDumped{};
    });
    const auto w_a_nested = MakeSelfContainedMetricLambda(storage, "a", {}, [](Writer& writer) {
        writer["s"]["d"].ValueWithLabels(MetricTypeThatMustBeSkipped{"first at 'a.s.d' path"}, {"a", "b"});
        writer["s"]["d"].ValueWithLabels(MetricTypeThatMustBeSkipped{"second at 'a.s.d' path"}, {{"c", "d"}});
        writer["d"].ValueWithLabels(MetricTypeThatMustBeSkipped{"first at 'a.d' path"}, {"a", "b"});
        writer["d"].ValueWithLabels(MetricTypeThatMustBeSkipped{"second at 'a.d' path"}, {{"c", "d"}});
    });

    const auto* const expected = "a_dump{} 42\n";
    EXPECT_EQ(expected, ToPrometheusFormatUntyped(storage, Request::MakeWithPath("a.dump")));
}

USERVER_NAMESPACE_END
