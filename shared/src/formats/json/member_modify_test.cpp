#include <gtest/gtest.h>

#include <userver/formats/json/exception.hpp>
#include <userver/formats/json/inline.hpp>
#include <userver/formats/json/serialize.hpp>
#include <userver/formats/json/value_builder.hpp>

#include <formats/common/member_modify_test.hpp>

USERVER_NAMESPACE_BEGIN

namespace {

const auto kDoc = formats::json::MakeObject(
    "key1", 1, "key2", "val", "key3", formats::json::MakeObject("sub", -1),
    "key4", formats::json::MakeArray(1, 2, 3), "key5", 10.5);

}  // namespace

template <>
struct MemberModify<formats::json::ValueBuilder> : public ::testing::Test {
  // XXX: not working from base class in clang-7 (decltype?)
  static_assert(
      std::is_assignable_v<
          decltype(*std::declval<formats::json::ValueBuilder>().begin()),
          formats::json::ValueBuilder>,
      "ValueBuilder iterators are assignable");

  MemberModify() : builder_(kDoc) {}

  static formats::json::Value GetValue(formats::json::ValueBuilder& bld) {
    auto v = bld.ExtractValue();
    bld = v;
    return v;
  }

  formats::json::Value GetBuiltValue() { return GetValue(builder_); }

  formats::json::ValueBuilder builder_;

  using ValueBuilder = formats::json::ValueBuilder;
  using Value = formats::json::Value;
  using Type = formats::json::Type;

  using ParseException = formats::json::ParseException;
  using TypeMismatchException = formats::json::TypeMismatchException;
  using OutOfBoundsException = formats::json::OutOfBoundsException;
  using MemberMissingException = formats::json::MemberMissingException;
  using Exception = formats::json::Exception;

  constexpr static auto FromString = formats::json::FromString;
};

INSTANTIATE_TYPED_TEST_SUITE_P(FormatsJson, MemberModify,
                               formats::json::ValueBuilder);

USERVER_NAMESPACE_END
