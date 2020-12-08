#include <formats/yaml/serialize.hpp>
#include <formats/yaml/serialize_container.hpp>
#include <formats/yaml/value.hpp>
#include <formats/yaml/value_builder.hpp>

#include <formats/common/value_test.hpp>

template <>
struct Parsing<formats::yaml::Value> : public ::testing::Test {
  constexpr static auto FromString = formats::yaml::FromString;
  using ParseException = formats::yaml::Value::ParseException;
};

INSTANTIATE_TYPED_TEST_SUITE_P(FormatsYaml, Parsing, formats::yaml::Value);

TEST(FormatsYaml, NullAsDefaulted) {
  using formats::yaml::FromString;
  auto yaml = FromString(R"({"nulled": ~})");

  EXPECT_EQ(yaml["nulled"].As<int>({}), 0);
  EXPECT_EQ(yaml["nulled"].As<std::vector<int>>({}), std::vector<int>{});

  EXPECT_EQ(yaml["nulled"].As<int>(42), 42);

  std::vector<int> value{4, 2};
  EXPECT_EQ(yaml["nulled"].As<std::vector<int>>(value), value);
}
