#include <formats/common/merge_test.hpp>

#include <gtest/gtest-typed-test.h>

#include <userver/formats/bson/serialize.hpp>
#include <userver/formats/bson/value.hpp>
#include <userver/formats/bson/value_builder.hpp>

USERVER_NAMESPACE_BEGIN

template <>
class FormatsMerge<formats::bson::Value> : public ::testing::Test {
 public:
  constexpr static auto CheckMerge =
      CheckJsonMerge<formats::bson::Value, formats::bson::FromJsonString>;
};

INSTANTIATE_TYPED_TEST_SUITE_P(BsonMerge, FormatsMerge, formats::bson::Value);

USERVER_NAMESPACE_END
