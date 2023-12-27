#include <gtest/gtest.h>
#include <userver/formats/universal/common_containers.hpp>
#include <userver/formats/json.hpp>


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
    SerializationConfig<SomeStruct>();

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
  const auto json =  formats::json::FromString("{\"field1\":10,\"field2\":100}");
  const auto json2 = formats::json::FromString("{\"field1\":10,\"field3\":100}");
  const auto json3 = formats::json::FromString("{\"field1\":10,\"field2\":\"100\"}");
  EXPECT_EQ((bool)formats::parse::TryParse(json,  formats::parse::To<SomeStruct>{}), true);
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
    SerializationConfig<SomeStruct2>()
    .With<"field1">({.Maximum = {}, .Minimum = {}, .Default = 114});


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
  constexpr SomeStruct2 valid{{114}, {}, {}};
  EXPECT_EQ(formats::parse::TryParse(json, formats::parse::To<SomeStruct2>{}), valid);
};


struct SomeStruct3 {
  std::unordered_map<std::string, int> field;
  auto operator==(const SomeStruct3& other) const noexcept {
    return this->field == other.field;
  };
};


template <>
inline constexpr auto formats::universal::kSerialization<SomeStruct3> =
    SerializationConfig<SomeStruct3>()
    .With<"field">({.Additional = true});

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
    SerializationConfig<SomeStruct4>()
    .With<"field">({.Maximum = 120, .Minimum = 10});




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
    SerializationConfig<SomeStruct5>()
    .With<"field">({.Pattern = kRegex<"^[0-9]+$">});

TEST(TryParse, Pattern) {
  const auto json = formats::json::FromString(R"({"field":"1234412"})");
  const auto json2 = formats::json::FromString(R"({"field":"abcdefgh"})");
  EXPECT_EQ((bool)formats::parse::TryParse(json, formats::parse::To<SomeStruct5>{}), true);
  EXPECT_EQ((bool)formats::parse::TryParse(json2, formats::parse::To<SomeStruct5>{}), false);
};



TEST(TryParse, null) {
  const auto json = formats::json::FromString("null");
  EXPECT_EQ((bool)formats::parse::TryParse(json, formats::parse::To<std::optional<int>>{}), true);
};

struct SomeStruct6 {
  std::vector<std::vector<int>> field;
};

template <>
inline constexpr auto formats::universal::kSerialization<SomeStruct6> =
    SerializationConfig<SomeStruct6>()
    .With<"field">({.MinimalElements = 2, .Items = {.MinimalElements = 1, .Items = {.Maximum = {}, .Minimum = 10}}});

TEST(TryParse, Arrays) {
  const auto json =  formats::json::FromString(R"({"field":[[10], [20]]})");
  const auto json2 = formats::json::FromString(R"({"field":[["10"], [20]]})");
  const auto json3 = formats::json::FromString(R"({"field":[[9], [20]]})");
  const auto json4 = formats::json::FromString(R"({"field":[[], []]})");
  EXPECT_EQ((bool)formats::parse::TryParse(json,  formats::parse::To<SomeStruct6>{}),  true);
  EXPECT_EQ((bool)formats::parse::TryParse(json2, formats::parse::To<SomeStruct6>{}), false);
  EXPECT_EQ((bool)formats::parse::TryParse(json3, formats::parse::To<SomeStruct6>{}), false);
  EXPECT_EQ((bool)formats::parse::TryParse(json4, formats::parse::To<SomeStruct6>{}), false);
};

struct SomeStruct7  {
  int value;
  std::vector<SomeStruct7> children;
  inline bool operator==(const SomeStruct7& other) const {
    return this->value == other.value && this->children == other.children;
  };
};

template <>
inline constexpr auto formats::universal::kSerialization<SomeStruct7> =
    SerializationConfig<SomeStruct7>();


TEST(Parse, Recursive) {
  SomeStruct7 valid{1, {{2, {}}}};
  const auto json = formats::json::FromString(R"({"value":1,"children":[{"value":2,"children":[]}]})");
  const auto fromJson = json.As<SomeStruct7>();
  EXPECT_EQ(fromJson, valid);
};


USERVER_NAMESPACE_END
