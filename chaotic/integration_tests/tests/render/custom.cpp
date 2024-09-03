#include <userver/utest/assert_macros.hpp>

#include <userver/formats/json/inline.hpp>
#include <userver/formats/json/value_builder.hpp>

#include <schemas/custom_cpp_type.hpp>

USERVER_NAMESPACE_BEGIN

TEST(Custom, Int) {
  auto json = formats::json::MakeObject("integer", 12);
  auto custom = json.As<ns::ObjWithCustom>();
  EXPECT_EQ(custom.integer, std::chrono::milliseconds(12));

  auto json_back = formats::json::ValueBuilder{custom}.ExtractValue();
  EXPECT_EQ(json_back, json);
}

TEST(Custom, String) {
  auto json = formats::json::MakeObject("string", "make love");
  auto custom = json.As<ns::ObjWithCustom>();
  EXPECT_EQ(custom.string, my::CustomString{"make love"});

  auto json_back = formats::json::ValueBuilder{custom}.ExtractValue();
  EXPECT_EQ(json_back, json);
}

TEST(Custom, Decimal) {
  auto json = formats::json::MakeObject("decimal", "12.3456789");
  auto custom = json.As<ns::ObjWithCustom>();
  EXPECT_EQ(custom.decimal, decimal64::Decimal<10>::FromBiased(1234567890, 8));

  auto json_back = formats::json::ValueBuilder{custom}.ExtractValue();
  EXPECT_EQ(json_back, json);
}

TEST(Custom, Boolean) {
  auto json = formats::json::MakeObject("boolean", true);
  auto custom = json.As<ns::ObjWithCustom>();
  EXPECT_EQ(custom.boolean, my::CustomBoolean{true});

  auto json_back = formats::json::ValueBuilder{custom}.ExtractValue();
  EXPECT_EQ(json_back, json);
}

TEST(Custom, Number) {
  auto json = formats::json::MakeObject("number", 1.23);
  auto custom = json.As<ns::ObjWithCustom>();
  EXPECT_EQ(custom.number, my::CustomNumber{1.23});

  auto json_back = formats::json::ValueBuilder{custom}.ExtractValue();
  EXPECT_EQ(json_back, json);
}

TEST(Custom, Object) {
  auto json = formats::json::MakeObject(
      "object", formats::json::MakeObject("foo", "bar"));
  auto custom = json.As<ns::ObjWithCustom>();
  EXPECT_EQ(custom.object, my::CustomObject{"bar"});

  auto json_back = formats::json::ValueBuilder{custom}.ExtractValue();
  EXPECT_EQ(json_back, json);
}

TEST(Custom, XCppContainer) {
  auto json = formats::json::MakeObject("std_array",
                                        formats::json::MakeArray("bar", "foo"));
  auto custom = json.As<ns::ObjWithCustom>();
  EXPECT_EQ(custom.std_array, (std::set<std::string>{"foo", "bar"}));

  auto json_back = formats::json::ValueBuilder{custom}.ExtractValue();
  EXPECT_EQ(json_back, json);
}

TEST(Custom, XCppType) {
  auto json = formats::json::MakeObject("custom_array",
                                        formats::json::MakeArray("foo", "bar"));
  auto custom = json.As<ns::ObjWithCustom>();
  EXPECT_EQ(custom.custom_array,
            (my::CustomArray<std::string>{std::set{"foo", "bar"}}));

  auto json_back = formats::json::ValueBuilder{custom}.ExtractValue();
  EXPECT_EQ(json_back, json);
}

TEST(Custom, OneOf) {
  auto json = formats::json::MakeObject("oneOf", 5);
  auto custom = json.As<ns::ObjWithCustom>();
  EXPECT_EQ(*custom.oneOf, my::CustomOneOf(5));

  auto json_back = formats::json::ValueBuilder{custom}.ExtractValue();
  EXPECT_EQ(json_back, json) << ToString(json_back) << " " << ToString(json);
}

TEST(Custom, OneOfWithDiscriminator) {
  auto json = formats::json::MakeObject(
      "oneOfWithDiscriminator",
      formats::json::MakeObject("type", "CustomStruct1", "field1", 3));
  auto custom = json.As<ns::ObjWithCustom>();
  EXPECT_EQ(custom.oneOfWithDiscriminator, my::CustomOneOfWithDiscriminator(3));

  auto json_back = formats::json::ValueBuilder{custom}.ExtractValue();
  EXPECT_EQ(json_back, json);
}

TEST(Custom, AllOf) {
  auto json = formats::json::MakeObject(
      "allOf", formats::json::MakeObject("field1", "foo", "field2", "bar"));
  auto custom = json.As<ns::ObjWithCustom>();
  EXPECT_EQ(custom.allOf, (my::CustomAllOf{"foo", "bar"}));

  auto json_back = formats::json::ValueBuilder{custom}.ExtractValue();
  EXPECT_EQ(json_back, json);
}

USERVER_NAMESPACE_END
