#include <userver/utils/statistics/portability_info.hpp>

#include <limits>

#include <userver/utils/statistics/storage.hpp>

#include <userver/utest/utest.hpp>

USERVER_NAMESPACE_BEGIN

namespace utils::statistics {

UTEST(MetricsPortabilityInfo, Inf) {
  Storage storage;
  auto holder = storage.RegisterWriter("a.inf", [](Writer& writer) {
    writer = std::numeric_limits<double>::infinity();
  });

  auto warnings = GetPortabilityWarnings(storage, {});

  ASSERT_EQ(1, warnings.size());
  const auto& [code, entries] = *warnings.begin();
  EXPECT_EQ(WarningCode::kInf, code);
  ASSERT_EQ(1, entries.size());
  EXPECT_EQ(entries[0].path, "a.inf");
}

UTEST(MetricsPortabilityInfo, NanQuiet) {
  Storage storage;
  auto holder = storage.RegisterWriter("a.nan", [](Writer& writer) {
    writer = std::numeric_limits<double>::quiet_NaN();
  });

  auto warnings = GetPortabilityWarnings(storage, {});

  ASSERT_EQ(1, warnings.size());
  const auto& [code, entries] = *warnings.begin();
  EXPECT_EQ(WarningCode::kNan, code);
  ASSERT_EQ(1, entries.size());
  EXPECT_EQ(entries[0].path, "a.nan");
}

UTEST(MetricsPortabilityInfo, NanSignaling) {
  Storage storage;
  auto holder = storage.RegisterWriter("a.nan", [](Writer& writer) {
    writer = std::numeric_limits<double>::signaling_NaN();
  });

  auto warnings = GetPortabilityWarnings(storage, {});

  ASSERT_EQ(1, warnings.size());
  const auto& [code, entries] = *warnings.begin();
  EXPECT_EQ(WarningCode::kNan, code);
  ASSERT_EQ(1, entries.size());
  EXPECT_EQ(entries[0].path, "a.nan");
}

UTEST(MetricsPortabilityInfo, TooManyLabels) {
  Storage storage;
  auto holder =
      storage.RegisterWriter("a.labels",
                             [](Writer& writer) {
                               writer.ValueWithLabels(42, {{"1", "1"},
                                                           {"2", "2"},
                                                           {"3", "3"},
                                                           {"4", "4"},
                                                           {"5", "5"},
                                                           {"6", "6"},
                                                           {"7", "7"},
                                                           {"8", "8"},
                                                           {"9", "9"}});
                             },
                             {{"0", "0"}});

  auto warnings = GetPortabilityWarnings(storage, {});

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

UTEST_P(PortabilityInfoLabels, ReservedLabels) {
  Storage storage;
  auto holder = storage.RegisterWriter("a.label", [](Writer& writer) {
    writer.ValueWithLabels(42, {GetParam().name, "1"});
  });

  auto warnings = GetPortabilityWarnings(storage, {});

  ASSERT_EQ(1, warnings.size());
  const auto& [code, entries] = *warnings.begin();
  EXPECT_EQ(GetParam().code, code);
  ASSERT_EQ(1, entries.size());
  EXPECT_EQ(entries[0].path, "a.label");
}

INSTANTIATE_UTEST_SUITE_P(
    Metrics, PortabilityInfoLabels,
    ::testing::Values(NameCode{"application",
                               WarningCode::kReservedLabelApplication},
                      NameCode{"project", WarningCode::kReservedLabelProject},
                      NameCode{"cluster", WarningCode::kReservedLabelCluster},
                      NameCode{"service", WarningCode::kReservedLabelService},
                      NameCode{"host", WarningCode::kReservedLabelHost},
                      NameCode{"group", WarningCode::kReservedLabelGroup},
                      NameCode{"sensor", WarningCode::kReservedLabelSensor}),
    [](const auto& warnings) { return std::string{warnings.param.name}; });

UTEST(MetricsPortabilityInfo, LabelNameLong) {
  Storage storage;
  auto holder = storage.RegisterWriter("a.name", [](Writer& writer) {
    std::string s;
    s.resize(201, '!');
    writer.ValueWithLabels(42, {s, "1"});
  });

  auto warnings = GetPortabilityWarnings(storage, {});

  ASSERT_EQ(1, warnings.size());
  const auto& [code, entries] = *warnings.begin();
  EXPECT_EQ(WarningCode::kLabelNameLength, code);
  ASSERT_EQ(1, entries.size());
  EXPECT_EQ(entries[0].path, "a.name");
}

UTEST(MetricsPortabilityInfo, LabelValueLong) {
  Storage storage;
  auto holder = storage.RegisterWriter("a.value", [](Writer& writer) {
    std::string s;
    s.resize(201, '!');
    writer.ValueWithLabels(42, {"1", s});
  });

  auto warnings = GetPortabilityWarnings(storage, {});

  ASSERT_EQ(1, warnings.size());
  const auto& [code, entries] = *warnings.begin();
  EXPECT_EQ(WarningCode::kLabelValueLength, code);
  ASSERT_EQ(1, entries.size());
  EXPECT_EQ(entries[0].path, "a.value");
}

UTEST(MetricsPortabilityInfo, PathLong) {
  Storage storage;
  auto holder = storage.RegisterWriter("a.value", [](Writer& writer) {
    std::string s;
    s.resize(201, '!');
    writer[s] = 42;
  });

  auto warnings = GetPortabilityWarnings(storage, {});

  ASSERT_EQ(1, warnings.size());
  const auto& [code, entries] = *warnings.begin();
  EXPECT_EQ(WarningCode::kPathLength, code);
  ASSERT_EQ(1, entries.size());
  EXPECT_EQ(entries[0].path.substr(0, 7), "a.value");
}

UTEST(MetricsPortabilityInfo, LabelsMismatch) {
  Storage storage;
  auto holder = storage.RegisterWriter("a.mismatch", [](Writer& writer) {
    writer.ValueWithLabels(42, {"a", "1"});
  });
  auto holder2 = storage.RegisterWriter("a.mismatch", [](Writer& writer) {
    writer.ValueWithLabels(42, {"b", "1"});
  });

  auto warnings = GetPortabilityWarnings(storage, {});

  ASSERT_EQ(1, warnings.size());
  const auto& [code, entries] = *warnings.begin();
  EXPECT_EQ(WarningCode::kLabelNameMismatch, code);
  ASSERT_EQ(1, entries.size());
  EXPECT_EQ(entries[0].path, "a.mismatch");
}

}  // namespace utils::statistics

USERVER_NAMESPACE_END
