#include <gtest/gtest.h>

#include <userver/formats/yaml/exception.hpp>
#include <userver/formats/yaml/serialize.hpp>
#include <userver/formats/yaml/value_builder.hpp>

#include <formats/common/member_modify_test.hpp>

USERVER_NAMESPACE_BEGIN

namespace {
constexpr const char* kDoc = R"(
key1: 1
key2: val
key3:
  sub: -1
key4: [1, 2, 3]
key5: 10.5
)";
}

template <>
struct MemberModify<formats::yaml::ValueBuilder> : public ::testing::Test {
  // XXX: not working from base class in clang-7 (decltype?)
  static_assert(
      std::is_assignable_v<
          decltype(*std::declval<formats::yaml::ValueBuilder>().begin()),
          formats::yaml::ValueBuilder>,
      "ValueBuilder iterators are assignable");

  MemberModify() : builder_(formats::yaml::FromString(kDoc)) {}

  static formats::yaml::Value GetValue(formats::yaml::ValueBuilder& bld) {
    auto v = bld.ExtractValue();
    bld = v;
    return v;
  }

  formats::yaml::Value GetBuiltValue() { return GetValue(builder_); }

  formats::yaml::ValueBuilder builder_;

  using ValueBuilder = formats::yaml::ValueBuilder;
  using Value = formats::yaml::Value;
  using Type = formats::yaml::Type;

  using ParseException = formats::yaml::ParseException;
  using TypeMismatchException = formats::yaml::TypeMismatchException;
  using OutOfBoundsException = formats::yaml::OutOfBoundsException;
  using MemberMissingException = formats::yaml::MemberMissingException;
  using Exception = formats::yaml::Exception;

  constexpr static auto FromString = formats::yaml::FromString;
};

INSTANTIATE_TYPED_TEST_SUITE_P(FormatsYaml, MemberModify,
                               formats::yaml::ValueBuilder);

USERVER_NAMESPACE_END
