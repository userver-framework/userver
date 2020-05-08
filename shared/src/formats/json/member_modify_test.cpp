#include <gtest/gtest.h>

#include <formats/json/exception.hpp>
#include <formats/json/serialize.hpp>
#include <formats/json/value_builder.hpp>

#include <formats/common/member_modify_test.hpp>

namespace {
constexpr const char* kDoc =
    "{\"key1\":1,\"key2\":\"val\",\"key3\":{\"sub\":-1},\"key4\":[1,2,3],"
    "\"key5\":10.5}";
}

template <>
struct MemberModify<formats::json::ValueBuilder> : public ::testing::Test {
  // XXX: not working from base class in clang-7 (decltype?)
  static_assert(
      std::is_assignable_v<
          decltype(*std::declval<formats::json::ValueBuilder>().begin()),
          formats::json::ValueBuilder>,
      "ValueBuilder iterators are assignable");

  MemberModify() : builder_(formats::json::FromString(kDoc)) {}

  formats::json::Value GetValue(formats::json::ValueBuilder& bld) {
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
