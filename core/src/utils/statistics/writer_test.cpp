#include <userver/utest/utest.hpp>

#include <userver/utils/statistics/graphite.hpp>
#include <userver/utils/statistics/prometheus.hpp>
#include <userver/utils/statistics/storage.hpp>

#include <userver/rcu/rcu_map.hpp>
#include <userver/utils/mock_now.hpp>

USERVER_NAMESPACE_BEGIN

namespace {
using utils::statistics::Request;
using utils::statistics::Storage;
using utils::statistics::ToPrometheusFormatUntyped;
using utils::statistics::Writer;

/// [DumpMetric basic]
struct ComponentMetricsNested {
  std::atomic<int> nested1{17};
  std::atomic<int> nested2{18};
};

void DumpMetric(utils::statistics::Writer& writer,
                const ComponentMetricsNested& m) {
  // Metric without labels
  writer["nested1"] = m.nested1;

  // Metric with label
  writer["nested2"].ValueWithLabels(m.nested2, {"label_name", "label_value"});
}
/// [DumpMetric basic]

/// [DumpMetric nested]
struct ComponentMetrics {
  rcu::RcuMap<std::string, ComponentMetricsNested> metrics;
};

void DumpMetric(utils::statistics::Writer& writer, const ComponentMetrics& m) {
  for (const auto& [key, value] : m.metrics) {
    UASSERT(value);
    writer.ValueWithLabels(*value, {{"a_label", key}, {"b_label", "b"}});
  }
}
/// [DumpMetric nested]

struct MetricTypeThatMayBeDumped {};
void DumpMetric(Writer& writer, MetricTypeThatMayBeDumped) { writer = 42; }

struct MetricTypeThatMustBeSkipped {
  const char* context;
};
void DumpMetric(Writer&, MetricTypeThatMustBeSkipped metric) {
  ADD_FAILURE() << "Metric dumping was called " << metric.context;
}

void DoTestBasic(Storage& storage) {
  const auto* const expected = "prefix_name{} 42\n";
  EXPECT_EQ(expected, ToPrometheusFormatUntyped(storage));
  EXPECT_EQ(expected, ToPrometheusFormatUntyped(
                          storage, Request::MakeWithPrefix("prefix")));
  EXPECT_EQ(expected,
            ToPrometheusFormatUntyped(storage, Request::MakeWithPrefix("pre")));

  auto holder0 =
      storage.RegisterWriter("prefix", [](Writer& writer) { writer = 41; });
  auto holder1 = storage.RegisterWriter("prEfix.name",
                                        [](Writer& writer) { writer = 43; });
  auto holder2 =
      storage.RegisterWriter({}, [](Writer& writer) { writer["a"] = 44; });

  EXPECT_EQ(expected, ToPrometheusFormatUntyped(
                          storage, Request::MakeWithPrefix("prefix.")));
  EXPECT_EQ(expected, ToPrometheusFormatUntyped(
                          storage, Request::MakeWithPrefix("prefix.name")));
  EXPECT_EQ(expected, ToPrometheusFormatUntyped(
                          storage, Request::MakeWithPath("prefix.name")));

  const auto* const expected_labeled = "prefix_name{lab=\"foo\"} 42\n";
  EXPECT_EQ(
      expected_labeled,
      ToPrometheusFormatUntyped(
          storage, Request::MakeWithPrefix("prefix.name", {{"lab", "foo"}})));

  EXPECT_EQ(expected_labeled, ToPrometheusFormatUntyped(
                                  storage, Request::MakeWithPrefix(
                                               "prefix.name", {{"lab", "foo"}},
                                               {{"lab", "foo"}})));
  const auto* const expected_all = R"(
prefix_name{} 42
prefix{} 41
prEfix_name{} 43
a{} 44
)";
  EXPECT_EQ(expected_all, "\n" + ToPrometheusFormatUntyped(storage, Request{}));
}

struct NotDumpable {};

namespace some {

struct Dumpable1 {};
void DumpMetric(utils::statistics::Writer&, const Dumpable1&) {}

struct Dumpable2 {};
void DumpMetric(utils::statistics::Writer&, Dumpable2) {}

struct Dumpable3 {};

}  // namespace some

}  // namespace

namespace utils::statistics {
void DumpMetric(Writer&, some::Dumpable3) {}
}  // namespace utils::statistics

UTEST(MetricsWriter, Basic1) {
  Storage storage;
  auto holder = storage.RegisterWriter("prefix.name",
                                       [](Writer& writer) { writer = 42; });
  DoTestBasic(storage);
}

UTEST(MetricsWriter, Basic2) {
  Storage storage;
  auto holder = storage.RegisterWriter(
      "", [](Writer& writer) { writer["prefix.name"] = 42; });

  DoTestBasic(storage);
}

UTEST(MetricsWriter, Basic3) {
  Storage storage;
  auto holder = storage.RegisterWriter("prefix", [](Writer& writer) {
    writer["name"] = 42;

    writer["a"] = some::Dumpable1{};
    writer["b"] = some::Dumpable2{};
    writer["c"] = some::Dumpable3{};
  });

  DoTestBasic(storage);
}

UTEST(MetricsWriter, Sample) {
  Storage storage;

  // The names mimic class member names, for the sample purposes
  ComponentMetrics component_metrics_;
  utils::statistics::Entry holder_;
  component_metrics_.metrics.Insert("metric-in-map",
                                    std::make_shared<ComponentMetricsNested>());
  constexpr std::chrono::seconds kDuration{1674810371};
  utils::datetime::MockNowSet(std::chrono::system_clock::time_point{kDuration});

  /// [DumpMetric RegisterWriter]
  holder_ = storage.RegisterWriter("begin-on-the-metric-path",
                                   [&](Writer& writer) {
                                     // Metric without labels
                                     writer["metric1"] = 42;

                                     ComponentMetrics& metrics =
                                         component_metrics_;
                                     writer["more_metrics"] = metrics;
                                   },
                                   {{"label_for_all", "labell value"}});
  /// [DumpMetric RegisterWriter]

  auto result = utils::statistics::ToGraphiteFormat(
      storage, Request::MakeWithPrefix("begin"));

  constexpr std::string_view ethalon = /** [metrics graphite] */ R"(
begin-on-the-metric-path.metric1;label_for_all=labell_value 42 1674810371
begin-on-the-metric-path.more_metrics.nested1;label_for_all=labell_value;a_label=metric-in-map;b_label=b 17 1674810371
begin-on-the-metric-path.more_metrics.nested2;label_for_all=labell_value;a_label=metric-in-map;b_label=b;label_name=label_value 18 1674810371
)"; /** [metrics graphite] */

  EXPECT_EQ(result, ethalon.substr(1));
}

#ifdef NDEBUG
// In debug this test should abort
UTEST(MetricsWriter, WithError) {
  Storage storage;
  auto holder = storage.RegisterWriter("prefix", [](Writer& writer) {
    writer["name"] = 42;
    throw std::runtime_error("Something went wrong");
  });

  // Make sure that exception in one metric does not skip written metrics
  DoTestBasic(storage);

  auto holder0 = storage.RegisterWriter("otherprefix", [](Writer& writer) {
    writer["foo"] = 41;
    std::string too_long_path(70000, '!');
    writer[too_long_path] = 411;
  });

  auto holder1 = storage.RegisterWriter("a", [](Writer& writer) {
    writer["b"] = 4;
    std::string too_long_path(70000, '?');
    writer[too_long_path].ValueWithLabels(412.0, {"hello", "world"});
  });

  auto holder2 = storage.RegisterWriter("x", [](Writer& writer) {
    writer["a"] = 14;

    auto b = writer["b"];
    auto c = writer["c"];
    b = 1.0;
    c = std::size_t{2};
  });

  std::string too_long_path(70000, 'a');
  auto holder3 = storage.RegisterWriter(
      too_long_path, [](Writer& writer) { writer["q"] = 24; });

  auto holder4 = storage.RegisterWriter({}, [](Writer& writer) {
    writer["some"]["path"].ValueWithLabels(3, {"name", "value"});
    writer["some"]["path"].ValueWithLabels(
        [](bool raise) {
          return raise ? throw std::runtime_error{"Oops"} : 1;
        }(true),
        {"name", "value"});
  });

  auto holder5 = storage.RegisterWriter("some",
                                        [](Writer& writer) {
                                          writer["a"].ValueWithLabels(7, {});

                                          auto b = writer["b"];
                                          auto c = writer["c"];
                                          b = 1UL;
                                        },
                                        {{"name2", "value2"}});

  auto holder6 = storage.RegisterWriter(
      "some",
      [](Writer& writer) {
        writer["b"].ValueWithLabels(8, {"name2", "value2"});
      },
      {{"name", "value"}});

  auto holder7 = storage.RegisterWriter({}, [](Writer& writer) {
    writer["some"]["path"]["1"] = 100500;
    writer = 0;
  });
  auto holder8 = storage.RegisterWriter("some", [](Writer& writer) {
    writer["path"]["2"] = 100502;
    writer[""] = 10;
  });
  auto holder9 = storage.RegisterWriter(
      "some", [](Writer& writer) { writer["path"]["3"] = 100503; });
  auto holder10 = storage.RegisterWriter(
      "some", [](Writer& writer) { writer["path"]["4"] = 100504; });
  auto holder11 = storage.RegisterWriter(
      "some", [](Writer& writer) { writer["path"]["5"] = 100505; });

  auto holderlast =
      storage.RegisterWriter("fine", [](Writer& writer) { writer["ok"] = 1; });

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
#endif  // NDEBUG

UTEST(MetricsWriter, LabeledSingle) {
  Storage storage;
  auto holder = storage.RegisterWriter("prefix", [](Writer& writer) {
    writer["name"].ValueWithLabels(42, {"label_name", "label_value1"});
    writer["name"].ValueWithLabels(43, {"label_name", "label_value2"});
  });

  auto holder0 = storage.RegisterWriter("prefix", [](Writer& writer) {
    writer.ValueWithLabels(52, {"label_name", "label_value1"});
    writer.ValueWithLabels(53, {"label_name", "label_value2"});
  });
  auto holder1 = storage.RegisterWriter("prEfix.name", [](Writer& writer) {
    writer.ValueWithLabels(62, {"label_name", "label_value1"});
    writer.ValueWithLabels(63, {"label_name", "label_value2"});
  });

  const auto* const expected_labeled =
      "prefix_name{label_name=\"label_value1\"} 42\n"
      "prefix_name{label_name=\"label_value2\"} 43\n";
  EXPECT_EQ(
      expected_labeled,
      ToPrometheusFormatUntyped(storage, Request::MakeWithPath("prefix.name")));

  const auto* const expected_labeled2 =
      "prefix_name{label_name=\"label_value2\"} 43\n";
  EXPECT_EQ(
      expected_labeled2,
      ToPrometheusFormatUntyped(
          storage, Request::MakeWithPath("prefix.name", {},
                                         {{"label_name", "label_value2"}})));
}

UTEST(MetricsWriter, LabeledMultiple) {
  Storage storage;

  utils::statistics::LabelView label{"name", "value"};

  auto holder = storage.RegisterWriter("a", [label](Writer& writer) {
    writer["1"].ValueWithLabels(1, {label, {"label_name", "label_value1"}});
    writer["1"].ValueWithLabels(2, {label, {"label_name", "label_value2"}});
  });

  auto holder0 = storage.RegisterWriter("a", [&label](Writer& writer) {
    writer.ValueWithLabels(52, {label, {"label_name", "label_value1"}});
    writer.ValueWithLabels(53, {label, {"label_name", "label_value2"}});
  });
  auto holder1 = storage.RegisterWriter("b", [label](Writer& writer) {
    writer.ValueWithLabels(62, {label, {"label_name", "label_value1"}});
    writer.ValueWithLabels(63, {label, {"label_name", "label_value2"}});
    writer.ValueWithLabels(63, {label, {"label_name", "label_value3"}});
  });
  auto holder2 = storage.RegisterWriter(
      "c",
      [](Writer& writer) {
        writer.ValueWithLabels(62, {{"label_name", "label_value1"}});
        writer.ValueWithLabels(63, {"label_name", "label_value2"});
        writer.ValueWithLabels(63, {{"label_name", "label_value3"}});
      },
      {{"name", "value"}});

  const auto* const expected_labeled =
      "a_1{name=\"value\",label_name=\"label_value1\"} 1\n"
      "a_1{name=\"value\",label_name=\"label_value2\"} 2\n";
  EXPECT_EQ(expected_labeled,
            ToPrometheusFormatUntyped(storage, Request::MakeWithPath("a.1")));

  const auto* const expected_labeled2 =
      "a_1{name=\"value\",label_name=\"label_value2\"} 2\n";
  EXPECT_EQ(expected_labeled2,
            ToPrometheusFormatUntyped(
                storage, Request::MakeWithPath(
                             "a.1", {}, {{"label_name", "label_value2"}})));
  EXPECT_EQ(
      expected_labeled2,
      ToPrometheusFormatUntyped(
          storage,
          Request::MakeWithPath(
              "a.1", {}, {{"name", "value"}, {"label_name", "label_value2"}})));

  const auto* const expected_labeled2_a =
      "a{name=\"value\",label_name=\"label_value2\"} 53\n";
  EXPECT_EQ(expected_labeled2_a,
            ToPrometheusFormatUntyped(
                storage, Request::MakeWithPath(
                             "a", {}, {{"label_name", "label_value2"}})));
  EXPECT_EQ(
      expected_labeled2_a,
      ToPrometheusFormatUntyped(
          storage,
          Request::MakeWithPath(
              "a", {}, {{"name", "value"}, {"label_name", "label_value2"}})));

  const auto* const expected_labeled2_c =
      "c{name=\"value\",label_name=\"label_value2\"} 63\n";
  EXPECT_EQ(expected_labeled2_c,
            ToPrometheusFormatUntyped(
                storage, Request::MakeWithPath(
                             "c", {}, {{"label_name", "label_value2"}})));
  EXPECT_EQ(
      expected_labeled2_c,
      ToPrometheusFormatUntyped(
          storage,
          Request::MakeWithPath(
              "c", {}, {{"name", "value"}, {"label_name", "label_value2"}})));
}

UTEST(MetricsWriter, CustomTypesOptimization) {
  Storage storage;
  auto holder = storage.RegisterWriter("skip", [](Writer& writer) {
    writer = MetricTypeThatMustBeSkipped{"at 'skip' path"};
  });
  auto holder1 = storage.RegisterWriter("a", [](Writer& writer) {
    writer["skip"] = MetricTypeThatMustBeSkipped{"at 'a.skip' path"};
  });
  auto holder2 = storage.RegisterWriter("a", [](Writer& writer) {
    writer["dump"] = MetricTypeThatMayBeDumped{};
  });
  auto holder3 = storage.RegisterWriter("a", [](Writer& writer) {
    writer["s"]["d"].ValueWithLabels(
        MetricTypeThatMustBeSkipped{"first at 'a.s.d' path"}, {"a", "b"});
    writer["s"]["d"].ValueWithLabels(
        MetricTypeThatMustBeSkipped{"second at 'a.s.d' path"}, {{"c", "d"}});

    writer["d"].ValueWithLabels(
        MetricTypeThatMustBeSkipped{"first at 'a.d' path"}, {"a", "b"});
    writer["d"].ValueWithLabels(
        MetricTypeThatMustBeSkipped{"second at 'a.d' path"}, {{"c", "d"}});
  });

  const auto* const expected = "a_dump{} 42\n";
  EXPECT_EQ(expected, ToPrometheusFormatUntyped(
                          storage, Request::MakeWithPath("a.dump")));

  // Manual unregister to avoid fake writer call when destroying a `Entry`
  holder3.Unregister();
  holder2.Unregister();
  holder1.Unregister();
  holder.Unregister();
}

UTEST(MetricsWriter, ComplexPathsStored) {
  Storage storage;
  auto holder = storage.RegisterWriter("a", [](Writer& writer) {
    auto new_writer = writer["b"]["c"]["d"];

    new_writer["e"] = 42;
  });

  const auto* const expected2 = "a_b_c_d_e{} 42\n";
  EXPECT_EQ(expected2, ToPrometheusFormatUntyped(storage));
}

UTEST(MetricsWriter, CustomizationPointChecks) {
  EXPECT_TRUE(utils::statistics::kHasWriterSupport<std::atomic<int>>);
  EXPECT_TRUE(utils::statistics::kHasWriterSupport<int>);
  EXPECT_TRUE(utils::statistics::kHasWriterSupport<long double>);

  EXPECT_FALSE(utils::statistics::kHasWriterSupport<NotDumpable>);
  EXPECT_FALSE(utils::statistics::kHasWriterSupport<std::string>);
  EXPECT_FALSE(utils::statistics::kHasWriterSupport<std::string_view>);

  EXPECT_TRUE(utils::statistics::kHasWriterSupport<some::Dumpable1>);
  EXPECT_TRUE(utils::statistics::kHasWriterSupport<some::Dumpable2>);
  EXPECT_TRUE(utils::statistics::kHasWriterSupport<some::Dumpable3>);
}

UTEST(MetricsWriter, AutomaticUnsubscribingCheckWriterData) {
  Storage storage;
  int counter = 0;
  auto writer_func = [&counter](Writer& writer) {
    writer = 1;
    writer["a"] = 2;
    counter++;
  };

  auto holder1 = storage.RegisterWriter("prefix1", writer_func);
  { auto holder2 = storage.RegisterWriter("prefix2", writer_func); }

  if constexpr (utils::statistics::impl::kCheckSubscriptionUB) {
    EXPECT_EQ(counter, 1);
  } else {
    EXPECT_EQ(counter, 0);
  }
}

UTEST(MetricsWriter, AutomaticUnsubscribingCheckExtenderData) {
  Storage storage;
  int counter = 0;
  auto extender_func = [&counter](const utils::statistics::StatisticsRequest&) {
    counter++;
    return formats::json::ValueBuilder{};
  };

  auto holder1 = storage.RegisterExtender("prefix1", extender_func);
  { auto holder2 = storage.RegisterExtender("prefix2", extender_func); }

  if constexpr (utils::statistics::impl::kCheckSubscriptionUB) {
    EXPECT_EQ(counter, 1);
  } else {
    EXPECT_EQ(counter, 0);
  }
}

USERVER_NAMESPACE_END
