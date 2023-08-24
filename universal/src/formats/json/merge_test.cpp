#include <formats/common/merge_test.hpp>

#include <gtest/gtest-typed-test.h>

#include <userver/formats/json/serialize.hpp>
#include <userver/formats/json/value.hpp>

USERVER_NAMESPACE_BEGIN

template <>
class FormatsMerge<formats::json::Value> : public ::testing::Test {
 public:
  constexpr static auto CheckMerge =
      CheckJsonMerge<formats::json::Value, formats::json::FromString>;
};

INSTANTIATE_TYPED_TEST_SUITE_P(JsonMerge, FormatsMerge, formats::json::Value);

USERVER_NAMESPACE_END
