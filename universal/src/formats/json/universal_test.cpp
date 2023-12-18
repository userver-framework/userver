#include <gtest/gtest.h>
#include <userver/formats/universal/common_checks.hpp>
#include <userver/formats/json.hpp>
#include <userver/formats/json/universal.hpp>


USERVER_NAMESPACE_BEGIN

struct SomeStruct {
  int field1;
  int field2;
  constexpr auto operator==(const SomeStruct& other) const noexcept {
    return (this->field1 == other.field1) && (this->field2 == other.field2);
  };
};

template <>
inline constexpr auto formats::universal::kSerialization<SomeStruct> =
    SerializationConfig<SomeStruct>::Create();

TEST(Serialize, Basic) {
  SomeStruct a{10, 100};
  const auto json = formats::json::ValueBuilder(a).ExtractValue();
  EXPECT_EQ(formats::json::ToString(json), "{\"field1\":10,\"field2\":100}");
};

TEST(Parse, Basic) {
  const auto json = formats::json::FromString("{\"field1\":10,\"field2\":100}");
  const auto fromJson = json.As<SomeStruct>();
  constexpr SomeStruct valid{10, 100};
  EXPECT_EQ(fromJson, valid);
};

TEST(TryParse, Basic) {
  const auto json = formats::json::FromString("{\"field1\":10,\"field2\":100}");
  const auto json2 = formats::json::FromString("{\"field1\":10,\"field3\":100}");
  const auto json3 = formats::json::FromString("{\"field1\":10,\"field2\":\"100\"}");
  EXPECT_EQ((bool)formats::parse::TryParse(json, formats::parse::To<SomeStruct>{}), true);
  EXPECT_EQ((bool)formats::parse::TryParse(json2, formats::parse::To<SomeStruct>{}), false);
  EXPECT_EQ((bool)formats::parse::TryParse(json3, formats::parse::To<SomeStruct>{}), false);
};


struct SomeStruct2 {
  std::optional<int> field1;
  std::optional<int> field2;
  std::optional<int> field3;
  constexpr bool operator==(const SomeStruct2& other) const noexcept {
    return this->field1 == other.field1 && this->field2 == other.field2 && this->field3 == other.field3;
  };
};

template <>
inline constexpr auto formats::universal::kSerialization<SomeStruct2> =
    SerializationConfig<SomeStruct2>::Create()
    .With<"field1">(Configurator<Default<114>>{});


TEST(Serialize, Optional) {
  SomeStruct2 a{{}, 100, {}};
  const auto json = formats::json::ValueBuilder(a).ExtractValue();
  EXPECT_EQ(json, formats::json::FromString("{\"field1\":114,\"field2\":100}"));
};

TEST(Parse, Optional) {
  constexpr SomeStruct2 valid{{114}, {}, {}};
  const auto json = formats::json::FromString("{}");
  EXPECT_EQ(json.As<SomeStruct2>(), valid);
};

TEST(TryParse, Optional) {
  const auto json = formats::json::FromString("{}");
  EXPECT_EQ((bool)formats::parse::TryParse(json, formats::parse::To<SomeStruct2>{}), true);
};


struct SomeStruct3 {
  std::unordered_map<std::string, int> field;
  auto operator==(const SomeStruct3& other) const noexcept {
    return this->field == other.field;
  };
};

struct SomeStruct3Description {
  formats::universal::Configurator<formats::universal::Additional> field;
};
template <>
inline constexpr auto formats::universal::kSerialization<SomeStruct3> =
    SerializationConfig<SomeStruct3>::Create()
    .FromStruct<SomeStruct3Description>();

TEST(Serialize, Additional) {
  std::unordered_map<std::string, int> value;
  value["data1"] = 1;
  value["data2"] = 2;
  SomeStruct3 a{value};
  const auto json = formats::json::ValueBuilder(a).ExtractValue();
  EXPECT_EQ(json, formats::json::FromString("{\"data1\":1,\"data2\":2}"));
};



TEST(Parse, Additional) {
  std::unordered_map<std::string, int> value;
  value["data1"] = 1;
  value["data2"] = 2;
  SomeStruct3 valid{value};
  const auto json = formats::json::FromString("{\"data1\":1,\"data2\":2}");
  const auto fromJson = json.As<SomeStruct3>();
  EXPECT_EQ(valid, fromJson);
};


TEST(TryParse, Additional) {
  const auto json = formats::json::FromString("{\"data1\":1,\"data2\":2}");
  EXPECT_EQ((bool)formats::parse::TryParse(json, formats::parse::To<SomeStruct3>{}), true);
};

struct SomeStruct4 {
  int field;
};

template <>
inline constexpr auto formats::universal::kSerialization<SomeStruct4> =
    SerializationConfig<SomeStruct4>::Create()
    .With<"field">(Configurator<Max<120>, Min<10>>{});




TEST(TryParse, MinMax) {
  const auto json = formats::json::FromString("{\"field\":1}");
  const auto json2 = formats::json::FromString("{\"field\":11}");
  const auto json3 = formats::json::FromString("{\"field\":121}");

  EXPECT_EQ((bool)formats::parse::TryParse(json, formats::parse::To<SomeStruct4>{}), false);
  EXPECT_EQ((bool)formats::parse::TryParse(json2, formats::parse::To<SomeStruct4>{}), true);
  EXPECT_EQ((bool)formats::parse::TryParse(json3, formats::parse::To<SomeStruct4>{}), false);
};



struct SomeStruct5 {
  std::string field;
};

template <>
inline constexpr auto formats::universal::kSerialization<SomeStruct5> =
    SerializationConfig<SomeStruct5>::Create()
    .With<"field">(Configurator<Pattern<"^[0-9]+$">>());

TEST(TryParse, Pattern) {
  const auto json = formats::json::FromString(R"({"field":"1234412"})");
  const auto json2 = formats::json::FromString(R"({"field":"abcdefgh"})");
  EXPECT_EQ((bool)formats::parse::TryParse(json, formats::parse::To<SomeStruct5>{}), true);
  EXPECT_EQ((bool)formats::parse::TryParse(json2, formats::parse::To<SomeStruct5>{}), false);
};

USERVER_NAMESPACE_END
