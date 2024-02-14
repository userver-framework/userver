#if __cplusplus >= 202002L
#include <gtest/gtest.h>
#include <userver/formats/universal/common_containers.hpp>
#include <userver/formats/json.hpp>


USERVER_NAMESPACE_BEGIN

struct SomeStruct {
  int field1;
  int field2;
  inline constexpr bool operator==(const SomeStruct& other) const = default;
};
template <>
inline constexpr auto formats::universal::kSerialization<SomeStruct> =
    SerializationConfig<SomeStruct>();

TEST(Serialize, Basic) {
  SomeStruct a{10, 100};
  const auto json = formats::json::ValueBuilder(a).ExtractValue();
  EXPECT_EQ(formats::json::ToString(json), R"({"field1":10,"field2":100})");
};

TEST(Parse, Basic) {
  const auto json = formats::json::FromString(R"({"field1":10,"field2":100})");
  const auto fromJson = json.As<SomeStruct>();
  constexpr SomeStruct valid{10, 100};
  EXPECT_EQ(fromJson, valid);
};

TEST(TryParse, Basic) {
  const auto json = formats::json::FromString(R"({"field1":10,"field2":100})");
  const auto json2 = formats::json::FromString(R"({"field1":10,"field3":100})");
  const auto json3 = formats::json::FromString(R"({"field1":10,"field2":"100"})");
  EXPECT_EQ((bool)formats::parse::TryParse(json,  formats::parse::To<SomeStruct>{}), true);
  EXPECT_EQ((bool)formats::parse::TryParse(json2, formats::parse::To<SomeStruct>{}), false);
  EXPECT_EQ((bool)formats::parse::TryParse(json3, formats::parse::To<SomeStruct>{}), false);
};


struct SomeStruct2 {
  std::optional<int> field1 = std::nullopt;
  std::optional<int> field2 = std::nullopt;
  std::optional<int> field3 = std::nullopt;
  std::optional<std::string> field4 = std::nullopt;
  constexpr bool operator==(const SomeStruct2& other) const noexcept {
    return this->field1 == other.field1 && this->field2 == other.field2 && this->field3 == other.field3 && this->field4 == other.field4;
  };
};

template <>
inline constexpr auto formats::universal::kSerialization<SomeStruct2> =
    SerializationConfig<SomeStruct2>()
    .With<"field1">({.Default = 114})
    .With<"field4">({.Default = {{"42"}}});


TEST(Serialize, Optional) {
  SomeStruct2 a{.field2 = 100};
  const auto json = formats::json::ValueBuilder(a).ExtractValue();
  EXPECT_EQ(json, formats::json::FromString(R"({"field1":114,"field2":100,"field4":"42"})"));
};
TEST(Parse, Optional) {
  SomeStruct2 valid{.field1 = 114, .field4 = {{"42"}}};
  const auto json = formats::json::FromString("{}");
  EXPECT_EQ(json.As<SomeStruct2>(), valid);
};

TEST(TryParse, Optional) {
  const auto json = formats::json::FromString("{}");
  SomeStruct2 valid{.field1 = 114, .field4 = {{"42"}}};
  EXPECT_EQ(formats::parse::TryParse(json, formats::parse::To<SomeStruct2>{}), valid);
};


struct SomeStruct3 {
  std::unordered_map<std::string, int> field;
  inline bool operator==(const SomeStruct3& other) const = default;
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
  EXPECT_EQ(json, formats::json::FromString(R"({"data1":1,"data2":2})"));
};

TEST(Parse, Additional) {
  std::unordered_map<std::string, int> value;
  value["data1"] = 1;
  value["data2"] = 2;
  SomeStruct3 valid{value};
  const auto json = formats::json::FromString(R"({"data1":1,"data2":2})");
  const auto fromJson = json.As<SomeStruct3>();
  EXPECT_EQ(valid, fromJson);
};

TEST(TryParse, Additional) {
  const auto json = formats::json::FromString(R"({"data1":1,"data2":2})");
  EXPECT_EQ((bool)formats::parse::TryParse(json, formats::parse::To<SomeStruct3>{}), true);
};

struct SomeStruct4 {
  int field;
  inline constexpr bool operator==(const SomeStruct4& other) const = default;
};

template <>
inline constexpr auto formats::universal::kSerialization<SomeStruct4> =
    SerializationConfig<SomeStruct4>()
    .With<"field">({.Maximum = 120, .Minimum = 10});




TEST(TryParse, MinMax) {
  const auto json =  formats::json::FromString(R"({"field":1})");
  const auto json2 = formats::json::FromString(R"({"field":11})");
  const auto json3 = formats::json::FromString(R"({"field":121})");

  EXPECT_EQ((bool)formats::parse::TryParse(json,  formats::parse::To<SomeStruct4>{}), false);
  EXPECT_EQ((bool)formats::parse::TryParse(json2, formats::parse::To<SomeStruct4>{}), true);
  EXPECT_EQ((bool)formats::parse::TryParse(json3, formats::parse::To<SomeStruct4>{}), false);
};



struct SomeStruct5 {
  std::string field;
  inline constexpr bool operator==(const SomeStruct5& other) const = default;
};

template <>
inline constexpr auto formats::universal::kSerialization<SomeStruct5> =
    SerializationConfig<SomeStruct5>()
    .With<"field">({.Pattern = kRegex<"^[0-9]+$">});

TEST(TryParse, Pattern) {
  const auto json =  formats::json::FromString(R"({"field":"1234412"})");
  const auto json2 = formats::json::FromString(R"({"field":"abcdefgh"})");
  EXPECT_EQ((bool)formats::parse::TryParse(json,  formats::parse::To<SomeStruct5>{}), true);
  EXPECT_EQ((bool)formats::parse::TryParse(json2, formats::parse::To<SomeStruct5>{}), false);
};



TEST(TryParse, null) {
  const auto json = formats::json::FromString("null");
  EXPECT_EQ((bool)formats::parse::TryParse(json, formats::parse::To<std::optional<int>>{}), true);
};

struct SomeStruct6 {
  std::vector<std::vector<int>> field;
  inline constexpr bool operator==(const SomeStruct6& other) const = default;
};

template <>
inline constexpr auto formats::universal::kSerialization<SomeStruct6> =
    SerializationConfig<SomeStruct6>()
    .With<"field">({.MinimalElements = 2, .Items = {.MinimalElements = 1, .Items = {.Minimum = 10}}});

TEST(TryParse, Arrays) {
  const auto json =  formats::json::FromString(R"({"field":[[10], [20]]})");
  const auto json2 = formats::json::FromString(R"({"field":[["10"], [20]]})");
  const auto json3 = formats::json::FromString(R"({"field":[[9], [20]]})");
  const auto json4 = formats::json::FromString(R"({"field":[[], []]})");
  EXPECT_EQ((bool)formats::parse::TryParse(json,  formats::parse::To<SomeStruct6>{}), true);
  EXPECT_EQ((bool)formats::parse::TryParse(json2, formats::parse::To<SomeStruct6>{}), false);
  EXPECT_EQ((bool)formats::parse::TryParse(json3, formats::parse::To<SomeStruct6>{}), false);
  EXPECT_EQ((bool)formats::parse::TryParse(json4, formats::parse::To<SomeStruct6>{}), false);
};

struct SomeStruct7  {
  int value;
  std::vector<SomeStruct7> children = {};
  inline constexpr bool operator==(const SomeStruct7& other) const = default;
};

template <>
inline constexpr auto formats::universal::kSerialization<SomeStruct7> =
    SerializationConfig<SomeStruct7>();


TEST(Parse, Recursive) {
  SomeStruct7 valid{.value = 1, .children = {{.value = 2}}};
  const auto json = formats::json::FromString(R"({"value":1,"children":[{"value":2,"children":[]}]})");
  const auto fromJson = json.As<SomeStruct7>();
  EXPECT_EQ(fromJson, valid);
};

struct SomeStruct8 {
  std::optional<int> field1;
  inline bool operator==(const SomeStruct8& other) const = default;
};

template <>
inline constexpr auto formats::universal::kSerialization<SomeStruct8> =
  SerializationConfig<SomeStruct8>()
  .With<"field1">({.Nullable = true});

struct SomeStruct9 {
  std::optional<int> field1;
  inline bool operator==(const SomeStruct9& other) const = default;
};

template <>
inline constexpr auto formats::universal::kSerialization<SomeStruct9> =
  SerializationConfig<SomeStruct9>();


TEST(Parse, Nullable) {
  EXPECT_EQ(formats::json::FromString(R"({"field1":null})").As<SomeStruct8>(),
      SomeStruct8{.field1 = std::nullopt});
  EXPECT_THROW(formats::json::FromString(R"({"field1":null})").As<SomeStruct9>(),
      formats::json::TypeMismatchException);
}

TEST(TryParse, Nullable) {
  EXPECT_EQ(formats::parse::TryParse(formats::json::FromString(R"({"field1":null})"), formats::parse::To<SomeStruct8>{}),
      SomeStruct8{.field1 = std::nullopt});
  EXPECT_EQ(formats::parse::TryParse(formats::json::FromString(R"({"field1":null})"), formats::parse::To<SomeStruct9>{}),
      std::nullopt);
}

struct SomeStruct10 {
  std::unordered_map<std::string, int> map;
  inline bool operator==(const SomeStruct10& other) const = default;
};

template <>
inline constexpr auto formats::universal::kSerialization<SomeStruct10> =
  SerializationConfig<SomeStruct10>();

TEST(Parse, Map) {
  const auto json = formats::json::FromString(R"({"map":{"1":1,"2":2}})");
  SomeStruct10 valid;
  valid.map.emplace("1", 1);
  valid.map.emplace("2", 2);
  EXPECT_EQ(json.As<SomeStruct10>(), valid);
}
TEST(Serialize, Map) {
  const auto valid = formats::json::FromString(R"({"map":{"1":1,"2":2}})");
  SomeStruct10 value;
  value.map.emplace("1", 1);
  value.map.emplace("2", 2);
  const auto json = formats::json::ValueBuilder(value).ExtractValue();
  EXPECT_EQ(json, valid);
}

struct SomeStruct11 {
  int field;
  inline constexpr bool operator==(const SomeStruct11& other) const = default;
};
template <typename Value>
auto Parse(Value&& value, formats::parse::To<SomeStruct11>) -> SomeStruct11 {
  return {.field = value["field"].template As<int>()};
}

template <typename Value>
auto Serialize(const SomeStruct11& data, formats::serialize::To<Value>) {
  typename Value::Builder builder;
  builder["field"] = data.field;
  return builder.ExtractValue();
}

struct SomeStruct12 {
  SomeStruct11 field;
  inline constexpr bool operator==(const SomeStruct12& other) const = default;
};

template <>
inline constexpr auto formats::universal::kSerialization<SomeStruct12> =
  SerializationConfig<SomeStruct12>();

TEST(Serialize, NonUniversalInUniversal) {
  const auto valid = formats::json::FromString(R"({"field":{"field":1}})");
  const auto json = formats::json::ValueBuilder(SomeStruct12{.field = {.field = 1}}).ExtractValue();
  EXPECT_EQ(json, valid);
};

TEST(Parse, NonUniversalInUniversal) {
  const auto json = formats::json::FromString(R"({"field":{"field":1}})");
  const SomeStruct12 valid{.field = {.field = 1}};
  EXPECT_EQ(json.As<SomeStruct12>(), valid);
};

USERVER_NAMESPACE_END
#endif
