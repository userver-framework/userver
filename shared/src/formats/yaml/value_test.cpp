#include <formats/yaml/serialize.hpp>
#include <formats/yaml/serialize_container.hpp>
#include <formats/yaml/value.hpp>

#include <formats/common/value_test.hpp>

template <>
struct Parsing<formats::yaml::Value> : public ::testing::Test {
  constexpr static auto FromString = formats::yaml::FromString;
  using ParseException = formats::yaml::Value::ParseException;
};

INSTANTIATE_TYPED_TEST_CASE_P(FormatsYaml, Parsing, formats::yaml::Value);
