#include <userver/formats/common/transfer_tag.hpp>

#include <utility>

#include <gtest/gtest.h>

#include <userver/formats/json/serialize.hpp>
#include <userver/formats/json/value.hpp>
#include <userver/formats/json/value_builder.hpp>
#include <userver/formats/yaml/serialize.hpp>
#include <userver/formats/yaml/value.hpp>
#include <userver/formats/yaml/value_builder.hpp>

USERVER_NAMESPACE_BEGIN

namespace {

using ValueTypes = ::testing::Types<formats::json::Value, formats::yaml::Value>;

template <typename T>
class ValueBuilderTransfer : public ::testing::Test {};

}  // namespace

using TransferTag = formats::common::TransferTag;

TYPED_TEST_SUITE(ValueBuilderTransfer, ValueTypes);

TYPED_TEST(ValueBuilderTransfer, Basic) {
  using ValueBuilder = typename TypeParam::Builder;
  ValueBuilder root;
  root["a"]["b"] = 1;
  ValueBuilder a(TransferTag(), std::move(root["a"]));
  a["b"] = 2;
  ASSERT_EQ(root.ExtractValue()["a"]["b"].template As<int>(), 2);
}

TYPED_TEST(ValueBuilderTransfer, AssignmentAffectsPointerToSame) {
  using ValueBuilder = typename TypeParam::Builder;
  ValueBuilder root;
  root["a"] = 1;
  ValueBuilder a1(TransferTag(), std::move(root["a"]));
  ValueBuilder a2(TransferTag(), std::move(root["a"]));
  a1 = 2;
  auto result = std::move(a2);  // copy to new root
  ASSERT_EQ(result.ExtractValue().template As<int>(), 2);
  ASSERT_EQ(root.ExtractValue()["a"].template As<int>(), 2);
}

USERVER_NAMESPACE_END
