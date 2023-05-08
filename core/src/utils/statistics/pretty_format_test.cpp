#include <userver/utils/statistics/pretty_format.hpp>

#include <userver/utest/utest.hpp>
#include <userver/utils/statistics/storage.hpp>

USERVER_NAMESPACE_BEGIN

namespace {

UTEST(MetricsPrettyFormat, CheckFormat) {
  utils::statistics::Storage storage;

  const auto entry = storage.RegisterWriter(
      "best_prefix",
      [](utils::statistics::Writer& writer) {
        writer.ValueWithLabels(42, {{"label_1", "value_1"},
                                    {"label_2", "value_2"},
                                    {"label_3", "value_3"}});
      },
      {});

  const auto request =
      utils::statistics::Request::MakeWithPrefix("best_prefix");
  constexpr auto expected =
      "best_prefix: label_1=value_1, label_2=value_2, "
      "label_3=value_3\tGAUGE\t42\n";
  const std::string result_str =
      utils::statistics::ToPrettyFormat(storage, request);
  EXPECT_EQ(result_str, expected);
}

UTEST(MetricsPrettyFormat, CheckSortingInFormat) {
  utils::statistics::Storage storage;

  const auto entry = storage.RegisterWriter(
      "best_prefix",
      [](utils::statistics::Writer& writer) {
        writer.ValueWithLabels(42, {{"label_3", "value_3"},
                                    {"label_2", "value_2"},
                                    {"label_1", "value_1"}});
      },
      {});

  const auto request =
      utils::statistics::Request::MakeWithPrefix("best_prefix");
  constexpr auto expected =
      "best_prefix: label_1=value_1, label_2=value_2, "
      "label_3=value_3\tGAUGE\t42\n";
  const std::string result_str =
      utils::statistics::ToPrettyFormat(storage, request);
  EXPECT_EQ(result_str, expected);
}

UTEST(MetricsPrettyFormat, NoLabels) {
  utils::statistics::Storage storage;

  const auto entry = storage.RegisterWriter(
      "best_prefix",
      [](utils::statistics::Writer& writer) { writer.ValueWithLabels(42, {}); },
      {});

  const auto request =
      utils::statistics::Request::MakeWithPrefix("best_prefix");
  constexpr auto expected = "best_prefix:\tGAUGE\t42\n";
  const std::string result_str =
      utils::statistics::ToPrettyFormat(storage, request);
  EXPECT_EQ(result_str, expected);
}

UTEST(MetricsPrettyFormat, Rate) {
  utils::statistics::Storage storage;

  const auto entry =
      storage.RegisterWriter("best_prefix",
                             [](utils::statistics::Writer& writer) {
                               writer = utils::statistics::Rate{42};
                             },
                             {});

  const auto request =
      utils::statistics::Request::MakeWithPrefix("best_prefix");
  constexpr auto expected = "best_prefix:\tRATE\t42\n";
  const std::string result_str =
      utils::statistics::ToPrettyFormat(storage, request);
  EXPECT_EQ(result_str, expected);
}

}  // namespace

USERVER_NAMESPACE_END
