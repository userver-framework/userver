#include <gtest/gtest.h>

#include <sstream>

#include <userver/formats/json/exception.hpp>
#include <userver/formats/json/inline.hpp>
#include <userver/formats/json/serialize.hpp>
#include <userver/formats/json/value_builder.hpp>

#include <formats/common/member_access_test.hpp>

USERVER_NAMESPACE_BEGIN

namespace {

const auto kDoc = formats::json::MakeObject(
    "key1", 1, "key2", "val", "key3", formats::json::MakeObject("sub", -1),
    "key4", formats::json::MakeArray(1, 2, 3), "key5", 10.5, "key6", false);

}  // namespace

template <>
struct MemberAccess<formats::json::Value> : public ::testing::Test {
  // XXX: not working from base class in clang-7 (decltype?)
  static_assert(!std::is_assignable_v<
                    decltype(*std::declval<formats::json::Value>().begin()),
                    formats::json::Value>,
                "Value iterators are not assignable");

  MemberAccess() : doc_(kDoc) {}
  formats::json::Value doc_;

  using ValueBuilder = formats::json::ValueBuilder;
  using Value = formats::json::Value;
  using Type = formats::json::Type;

  using ParseException = formats::json::ParseException;
  using TypeMismatchException = formats::json::TypeMismatchException;
  using OutOfBoundsException = formats::json::OutOfBoundsException;
  using MemberMissingException = formats::json::MemberMissingException;
};

INSTANTIATE_TYPED_TEST_SUITE_P(FormatsJson, MemberAccess, formats::json::Value);

struct FormatsJsonSpecificMemberAccess
    : public MemberAccess<formats::json::Value> {};

TEST_F(FormatsJsonSpecificMemberAccess, CheckPrimitiveTypes) {
  using TypeMismatchException = formats::json::TypeMismatchException;

  // Differs from YAML:
  EXPECT_FALSE(doc_["key3"]["sub"].IsUInt64());
  EXPECT_THROW(doc_["key1"].As<std::string>(), TypeMismatchException);
  EXPECT_THROW(doc_["key5"].As<std::string>(), TypeMismatchException);
  EXPECT_THROW(doc_["key6"].As<std::string>(), TypeMismatchException);
}

TEST_F(FormatsJsonSpecificMemberAccess, Items) {
  for ([[maybe_unused]] const auto& [key, value] : Items(doc_)) {
  }
}

USERVER_NAMESPACE_END
