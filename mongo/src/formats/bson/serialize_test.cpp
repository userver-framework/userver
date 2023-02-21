#include <userver/formats/bson/serialize.hpp>

#include <chrono>
#include <cstdint>
#include <limits>
#include <string>

#include <fmt/format.h>
#include <boost/algorithm/string/replace.hpp>

#include <userver/formats/bson.hpp>
#include <userver/formats/json.hpp>
#include <userver/utest/utest.hpp>

USERVER_NAMESPACE_BEGIN

namespace {

const std::string kJson = R"({
  "string": "test",
  "int": 1,
  "double": 1.0,
  "date": {"$date": "1970-01-01T00:00:00.001Z"}
})";

const std::string kRelaxedJson = R"({
  "string": "test",
  "int": 1,
  "double": 1.0,
  "inf": {"$numberDouble": "Infinity"},
  "date": {"$date": "1970-01-01T00:00:00.001Z"}
})";

const auto kTimePoint =
    std::chrono::system_clock::from_time_t(0) + std::chrono::milliseconds{1};

const auto kDoc = formats::bson::MakeDoc(
    "string", "test", "int", int64_t{1}, "double", 1.0, "inf",
    std::numeric_limits<double>::infinity(), "date", kTimePoint);

const std::string kArrayJson = R"([
  "test",
  1,
  1.0,
  {"$date": "1970-01-01T00:00:00.001Z"}
])";

const std::string kLegacyArrayJson = R"([
  "test",
  1,
  1.0,
  {"$date": 1}
])";

const auto kArray =
    formats::bson::MakeArray("test", int64_t{1}, 1.0, kTimePoint);

}  // namespace

TEST(Serialize, FromJson) {
  UEXPECT_THROW(formats::bson::FromJsonString(""),
                formats::bson::ParseException);
  UEXPECT_THROW(formats::bson::FromJsonString("[]"),
                formats::bson::ParseException);
  UEXPECT_THROW(formats::bson::FromJsonString(kArrayJson),
                formats::bson::ParseException);

  auto doc = formats::bson::FromJsonString(kJson);
  ASSERT_TRUE(doc["string"].IsString());
  EXPECT_EQ("test", doc["string"].As<std::string>());
  ASSERT_TRUE(doc["int"].IsInt32());
  EXPECT_EQ(1, doc["int"].As<int>());
  ASSERT_TRUE(doc["double"].IsDouble());
  EXPECT_DOUBLE_EQ(1.0, doc["double"].As<double>());
  ASSERT_TRUE(doc["date"].IsDateTime());
  EXPECT_EQ(kTimePoint,
            doc["date"].As<std::chrono::system_clock::time_point>());
}

TEST(Serialize, ArrayFromJson) {
  UEXPECT_THROW(formats::bson::ArrayFromJsonString(""),
                formats::bson::ParseException);
  UEXPECT_THROW(formats::bson::ArrayFromJsonString("{}"),
                formats::bson::ParseException);
  UEXPECT_THROW(formats::bson::ArrayFromJsonString(kJson),
                formats::bson::ParseException);

  auto array = formats::bson::ArrayFromJsonString(kArrayJson);
  ASSERT_EQ(4, array.GetSize());
  ASSERT_TRUE(array[0].IsString());
  EXPECT_EQ("test", array[0].As<std::string>());
  ASSERT_TRUE(array[1].IsInt32());
  EXPECT_EQ(1, array[1].As<int>());
  ASSERT_TRUE(array[2].IsDouble());
  EXPECT_DOUBLE_EQ(1.0, array[2].As<double>());
  ASSERT_TRUE(array[3].IsDateTime());
  EXPECT_EQ(kTimePoint, array[3].As<std::chrono::system_clock::time_point>());
}

TEST(Serialize, ToCanonicalJson) {
  auto json = formats::json::FromString(
      formats::bson::ToCanonicalJsonString(kDoc).GetView());

  ASSERT_TRUE(json["string"].IsString());
  EXPECT_EQ("test", json["string"].As<std::string>());
  ASSERT_TRUE(json["int"]["$numberLong"].IsString());
  EXPECT_EQ("1", json["int"]["$numberLong"].As<std::string>());
  ASSERT_TRUE(json["double"]["$numberDouble"].IsString());
  EXPECT_EQ("1.0", json["double"]["$numberDouble"].As<std::string>());
  ASSERT_TRUE(json["inf"]["$numberDouble"].IsString());
  EXPECT_EQ("Infinity", json["inf"]["$numberDouble"].As<std::string>());
  ASSERT_TRUE(json["date"]["$date"]["$numberLong"].IsString());
  EXPECT_EQ("1", json["date"]["$date"]["$numberLong"].As<std::string>());
}

TEST(Serialize, ToRelaxedJson) {
  auto json = formats::json::FromString(
      formats::bson::ToRelaxedJsonString(kDoc).GetView());

  ASSERT_TRUE(json["string"].IsString());
  EXPECT_EQ("test", json["string"].As<std::string>());
  ASSERT_TRUE(json["int"].IsInt64());
  EXPECT_EQ(1, json["int"].As<int64_t>());
  ASSERT_TRUE(json["double"].IsDouble());
  EXPECT_DOUBLE_EQ(1.0, json["double"].As<double>());
  ASSERT_TRUE(json["inf"]["$numberDouble"].IsString());
  EXPECT_EQ("Infinity", json["inf"]["$numberDouble"].As<std::string>());
  ASSERT_TRUE(json["date"]["$date"].IsString());
  EXPECT_EQ("1970-01-01T00:00:00.001Z",
            json["date"]["$date"].As<std::string>());
}

TEST(Serialize, ToLegacyJson) {
  auto str = formats::bson::ToLegacyJsonString(kDoc).ToString();
  // legacy json uses non-standard tokens for special double values
  boost::algorithm::replace_all(str, "inf", "0");
  auto json = formats::json::FromString(str);

  ASSERT_TRUE(json["string"].IsString());
  EXPECT_EQ("test", json["string"].As<std::string>());
  ASSERT_TRUE(json["int"].IsInt64());
  EXPECT_EQ(1, json["int"].As<int64_t>());
  ASSERT_TRUE(json["double"].IsDouble());
  EXPECT_DOUBLE_EQ(1.0, json["double"].As<double>());
  // ASSERT_TRUE(json["inf"].IsDouble());
  // EXPECT_TRUE(std::isinf(json["inf"].As<double>()));
  ASSERT_TRUE(json["date"]["$date"].IsInt64());
  EXPECT_EQ(1, json["date"]["$date"].As<int64_t>());
}

TEST(Serialize, ToArrayJson) {
  UEXPECT_THROW(formats::bson::ToArrayJsonString(kDoc),
                formats::bson::TypeMismatchException);

  auto json = formats::json::FromString(
      formats::bson::ToArrayJsonString(kArray).GetView());

  ASSERT_EQ(4, json.GetSize());
  ASSERT_TRUE(json[0].IsString());
  EXPECT_EQ("test", json[0].As<std::string>());
  ASSERT_TRUE(json[1].IsInt());
  EXPECT_EQ(1, json[1].As<int>());
  ASSERT_TRUE(json[2].IsDouble());
  EXPECT_DOUBLE_EQ(1.0, json[2].As<double>());
  ASSERT_TRUE(json[3]["$date"].IsInt64());
  EXPECT_EQ(1, json[3]["$date"].As<int64_t>());
}

TEST(Serialize, Fmt) {
  const auto doc_as_json = fmt::to_string(kDoc);
  EXPECT_EQ(formats::json::FromString(doc_as_json),
            formats::json::FromString(kRelaxedJson));

  const auto array_as_json =
      fmt::to_string(formats::bson::ToArrayJsonString(kArray));
  EXPECT_EQ(formats::json::FromString(array_as_json),
            formats::json::FromString(kLegacyArrayJson));
}

USERVER_NAMESPACE_END
