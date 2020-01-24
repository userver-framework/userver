#include <formats/json/serialize.hpp>
#include <formats/json/serialize_container.hpp>
#include <formats/json/value.hpp>
#include <formats/json/value_builder.hpp>
#include <formats/yaml/serialize.hpp>
#include <formats/yaml/value.hpp>

#include <boost/optional.hpp>
#include <boost/optional/optional_io.hpp>

#include <utils/strong_typedef.hpp>

#include <gtest/gtest.h>

struct IntTag {};
struct OptionalIntTag {};
using IntTypedef = utils::StrongTypedef<IntTag, int>;

using OptionalIntTypedef =
    utils::StrongTypedef<OptionalIntTag, boost::optional<int>>;

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
