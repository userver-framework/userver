#include <formats/common/merge_test.hpp>

#include <gtest/gtest-typed-test.h>

#include <userver/formats/yaml/serialize.hpp>
#include <userver/formats/yaml/value.hpp>

USERVER_NAMESPACE_BEGIN

template <>
class FormatsMerge<formats::yaml::Value> : public ::testing::Test {
 public:
  constexpr static auto CheckMerge =
      CheckJsonMerge<formats::yaml::Value, formats::yaml::FromString>;
};

INSTANTIATE_TYPED_TEST_SUITE_P(YamlMerge, FormatsMerge, formats::yaml::Value);

USERVER_NAMESPACE_END
