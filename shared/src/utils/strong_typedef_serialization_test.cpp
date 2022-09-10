#include <userver/formats/json/serialize.hpp>

#include <fmt/format.h>
#include <userver/utils/fmt_compat.hpp>

#include <userver/formats/json/serialize_container.hpp>
#include <userver/formats/json/value.hpp>
#include <userver/formats/json/value_builder.hpp>
#include <userver/formats/parse/boost_optional.hpp>
#include <userver/formats/serialize/boost_optional.hpp>
#include <userver/formats/yaml/serialize.hpp>
#include <userver/formats/yaml/value.hpp>

#include <array>
#include <limits>

#include <boost/optional.hpp>
#include <boost/optional/optional_io.hpp>

#include <userver/utils/strong_typedef.hpp>

#include <gtest/gtest.h>

USERVER_NAMESPACE_BEGIN

namespace utils {

template <class Tag, class T, utils::StrongTypedefOps Ops>
static void PrintTo(const utils::StrongTypedef<Tag, T, Ops>& v,
                    std::ostream* os) {
  ::testing::internal::UniversalTersePrint(v.GetUnderlying(), os);
}

}  // namespace utils

struct IntTag {};
struct OptionalIntTag {};
using IntTypedef = utils::StrongTypedef<IntTag, int>;

using OptionalIntTypedef =
    utils::StrongTypedef<OptionalIntTag, boost::optional<int>>;

namespace {

struct NonStreamable {};

struct MyNonStreamable final
    : utils::StrongTypedef<MyNonStreamable, NonStreamable,
                           utils::StrongTypedefOps::kCompareTransparent> {
  using StrongTypedef::StrongTypedef;
};

}  // namespace

USERVER_NAMESPACE_END

namespace fmt {

// fmt::format custoimization
template <>
struct formatter<USERVER_NAMESPACE::NonStreamable> : formatter<const char*> {
  template <typename FormatContext>
  auto format(const USERVER_NAMESPACE::NonStreamable& /*v*/,
              FormatContext& ctx) USERVER_FMT_CONST {
    return formatter<const char*>::format("!!!", ctx);
  }
};

}  // namespace fmt

USERVER_NAMESPACE_BEGIN

TEST(SerializeStrongTypedef, ParseInt) {
  auto json_object = formats::json::FromString(R"json({"data" : 10})json");
  auto json_data = json_object["data"];

  auto value = json_data.As<IntTypedef>();

  EXPECT_EQ(10, value.GetUnderlying());
}

TEST(SerializeStrongTypedef, ParseOptionalInt) {
  auto json_object = formats::json::FromString(R"json({"data" : 10})json");
  auto json_data = json_object["data"];

  auto value = json_data.As<OptionalIntTypedef>();

  ASSERT_TRUE(value.GetUnderlying());
  EXPECT_EQ(10, *(value.GetUnderlying()));
}

TEST(SerializeStrongTypedef, ParseOptionalIntNone) {
  auto json_object = formats::json::FromString(R"json({"data" : null})json");
  auto json_data = json_object["data"];

  auto value = json_data.As<OptionalIntTypedef>();

  EXPECT_FALSE(value.GetUnderlying());
}

TEST(SerializeStrongTypedef, SerializeCycleInt) {
  IntTypedef reference{10};
  // Serialize
  auto json_object = formats::json::ValueBuilder(reference).ExtractValue();

  // Parse
  auto test = json_object.As<IntTypedef>();

  EXPECT_EQ(reference, test);
}

TEST(SerializeStrongTypedef, SerializeCycleOptionalInt) {
  OptionalIntTypedef reference;
  reference = OptionalIntTypedef{boost::optional<int>{10}};

  // Serialize
  auto json_object = formats::json::ValueBuilder(reference).ExtractValue();

  // Parse
  auto test = json_object.As<OptionalIntTypedef>();

  EXPECT_EQ(reference, test);
}

TEST(SerializeStrongTypedef, SerializeCycleOptionalIntNone) {
  OptionalIntTypedef reference{boost::none};
  // Serialize
  auto json_object = formats::json::ValueBuilder(reference).ExtractValue();

  // Parse
  auto test = json_object.As<OptionalIntTypedef>();

  EXPECT_EQ(reference, test);
}

TEST(SerializeStrongTypedef, Fmt) {
  EXPECT_EQ("42", fmt::format("{}", IntTypedef{42}));
  EXPECT_EQ("f", fmt::format("{:x}", IntTypedef{15}));
  EXPECT_EQ("!!!", fmt::format("{}", MyNonStreamable{}));
}

TEST(SerializeStrongTypedef, FmtJoin) {
  EXPECT_EQ(
      "!!!,!!!",
      fmt::format("{}", fmt::join(std::array<MyNonStreamable, 2>{}, ",")));
}

TEST(SerializeStrongTypedef, ToString) {
  using StringTypedefCompareStrong =
      utils::StrongTypedef<struct StringTagStrong, std::string,
                           utils::StrongTypedefOps::kCompareStrong>;
  using StringTypedefCompareTransparent =
      utils::StrongTypedef<struct StringTagTransparent, std::string,
                           utils::StrongTypedefOps::kCompareTransparent>;
  using StringTypedefCompareTransparentOnly =
      utils::StrongTypedef<struct StringTagTransparentOnly, std::string,
                           utils::StrongTypedefOps::kCompareTransparentOnly>;

  using UInt64Typedef = utils::StrongTypedef<struct UInt64Tag, uint64_t>;

  EXPECT_EQ("qwerty", ToString(StringTypedefCompareStrong("qwerty")));
  EXPECT_EQ("qwerty", ToString(StringTypedefCompareTransparent("qwerty")));
  EXPECT_EQ("qwerty", ToString(StringTypedefCompareTransparentOnly("qwerty")));

  EXPECT_EQ("42", ToString(IntTypedef(42)));

  EXPECT_EQ(std::to_string(std::numeric_limits<uint64_t>::max()),
            ToString(UInt64Typedef(std::numeric_limits<uint64_t>::max())));
}

USERVER_NAMESPACE_END
