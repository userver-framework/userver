#include <gtest/gtest.h>

#include <chrono>
#include <cstdint>
#include <limits>
#include <string>

#include <boost/algorithm/string/replace.hpp>

#include <formats/bson.hpp>
#include <formats/bson/serialize.hpp>
#include <formats/json.hpp>

namespace {

const std::string kJson = R"({
  "string": "test",
  "int": 1,
  "double": 1.0,
  "date": { "$date": "1970-01-01T00:00:00.001Z" }
})";

const auto kTimePoint =
    std::chrono::system_clock::from_time_t(0) + std::chrono::milliseconds{1};

const auto kDoc = formats::bson::MakeDoc(
    "string", "test", "int", int64_t{1}, "double", 1.0, "inf",
    std::numeric_limits<double>::infinity(), "date", kTimePoint);

}  // namespace

TEST(Serialize, FromJson) {
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

TEST(Serialize, ToCanonicalJson) {
  auto json =
      formats::json::FromString(formats::bson::ToCanonicalJsonString(kDoc));

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
  auto json =
      formats::json::FromString(formats::bson::ToRelaxedJsonString(kDoc));

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
