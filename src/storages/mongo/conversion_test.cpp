#include <gtest/gtest.h>

#include <algorithm>
#include <chrono>
#include <string>
#include <vector>

#include <bsoncxx/array/value.hpp>
#include <bsoncxx/document/value.hpp>
#include <bsoncxx/json.hpp>
#include <bsoncxx/oid.hpp>

#include <storages/mongo/mongo.hpp>

namespace sm = storages::mongo;

TEST(MongoConversion, Missing) {
  auto test_elem = [](const auto& elem) {
    ASSERT_TRUE(sm::IsOneOf(elem, sm::ElementKind::kMissing));
    EXPECT_FALSE(
        sm::IsOneOf(elem, {sm::ElementKind::kNull, sm::ElementKind::kBool,
                           sm::ElementKind::kNumber, sm::ElementKind::kString,
                           sm::ElementKind::kTimestamp, sm::ElementKind::kArray,
                           sm::ElementKind::kDocument, sm::ElementKind::kOid}));

    EXPECT_DOUBLE_EQ(0.0, sm::ToDouble(elem));
    EXPECT_EQ(0, sm::ToInt64(elem));
    EXPECT_THROW(sm::ToTimePoint(elem), sm::BadType);
    EXPECT_THROW(sm::ToOid(elem), sm::BadType);
    EXPECT_TRUE(sm::ToString(elem).empty());
    EXPECT_FALSE(sm::ToBool(elem));
    EXPECT_TRUE(sm::ToArray(elem).empty());
    EXPECT_TRUE(sm::ToDocument(elem).empty());
    EXPECT_TRUE(sm::ToStringArray(elem).empty());
  };

  const auto doc = bsoncxx::from_json(R"({ "a": [] })");
  test_elem(doc.view()["a"][0]);
  test_elem(doc.view()["e"]);
}

TEST(MongoConversion, Null) {
  auto test_elem = [](const auto& elem) {
    ASSERT_TRUE(sm::IsOneOf(elem, sm::ElementKind::kNull));
    EXPECT_FALSE(
        sm::IsOneOf(elem, {sm::ElementKind::kMissing, sm::ElementKind::kBool,
                           sm::ElementKind::kNumber, sm::ElementKind::kString,
                           sm::ElementKind::kTimestamp, sm::ElementKind::kArray,
                           sm::ElementKind::kDocument, sm::ElementKind::kOid}));

    EXPECT_DOUBLE_EQ(0.0, sm::ToDouble(elem));
    EXPECT_EQ(0, sm::ToInt64(elem));
    EXPECT_THROW(sm::ToTimePoint(elem), sm::BadType);
    EXPECT_THROW(sm::ToOid(elem), sm::BadType);
    EXPECT_TRUE(sm::ToString(elem).empty());
    EXPECT_FALSE(sm::ToBool(elem));
    EXPECT_TRUE(sm::ToArray(elem).empty());
    EXPECT_TRUE(sm::ToDocument(elem).empty());
    EXPECT_TRUE(sm::ToStringArray(elem).empty());
  };

  const auto doc = bsoncxx::from_json(R"({ "a": [null], "e": null })");
  test_elem(doc.view()["a"][0]);
  test_elem(doc.view()["e"]);
}

TEST(MongoConversion, Bool) {
  auto test_elem = [](const auto& elem, bool value) {
    ASSERT_TRUE(sm::IsOneOf(elem, sm::ElementKind::kBool));
    EXPECT_FALSE(
        sm::IsOneOf(elem, {sm::ElementKind::kMissing, sm::ElementKind::kNull,
                           sm::ElementKind::kNumber, sm::ElementKind::kString,
                           sm::ElementKind::kTimestamp, sm::ElementKind::kArray,
                           sm::ElementKind::kDocument, sm::ElementKind::kOid}));

    EXPECT_DOUBLE_EQ(value, sm::ToDouble(elem));
    EXPECT_EQ(value, sm::ToInt64(elem));
    EXPECT_THROW(sm::ToTimePoint(elem), sm::BadType);
    EXPECT_THROW(sm::ToOid(elem), sm::BadType);
    if (value) {
      EXPECT_EQ("true", sm::ToString(elem));
    } else {
      EXPECT_TRUE(sm::ToString(elem).empty());
    }
    EXPECT_EQ(value, sm::ToBool(elem));
    EXPECT_THROW(sm::ToArray(elem), sm::BadType);
    EXPECT_THROW(sm::ToDocument(elem), sm::BadType);
    EXPECT_THROW(sm::ToStringArray(elem), sm::BadType);
  };

  const auto doc =
      bsoncxx::from_json(R"({ "a": [false, true], "ef": false, "et": true })");
  test_elem(doc.view()["a"][0], false);
  test_elem(doc.view()["a"][1], true);
  test_elem(doc.view()["ef"], false);
  test_elem(doc.view()["et"], true);
}

TEST(MongoConversion, Double) {
  auto test_elem = [](const auto& elem, double value) {
    ASSERT_TRUE(sm::IsOneOf(elem, sm::ElementKind::kNumber));
    EXPECT_FALSE(
        sm::IsOneOf(elem, {sm::ElementKind::kMissing, sm::ElementKind::kNull,
                           sm::ElementKind::kBool, sm::ElementKind::kString,
                           sm::ElementKind::kTimestamp, sm::ElementKind::kArray,
                           sm::ElementKind::kDocument, sm::ElementKind::kOid}));

    EXPECT_DOUBLE_EQ(value, sm::ToDouble(elem));
    EXPECT_EQ(static_cast<int64_t>(value), sm::ToInt64(elem));
    EXPECT_THROW(sm::ToTimePoint(elem), sm::BadType);
    EXPECT_THROW(sm::ToOid(elem), sm::BadType);
    EXPECT_EQ(std::to_string(value), sm::ToString(elem));
    EXPECT_EQ(!!value, sm::ToBool(elem));
    EXPECT_THROW(sm::ToArray(elem), sm::BadType);
    EXPECT_THROW(sm::ToDocument(elem), sm::BadType);
    EXPECT_THROW(sm::ToStringArray(elem), sm::BadType);
  };

  const auto doc = bsoncxx::from_json(R"({
        "a": [0.0, 0.123, -0.123],
        "ez": 0.0,
        "ep": 3.21,
        "en": -3.21
      })");
  test_elem(doc.view()["a"][0], 0.0);
  test_elem(doc.view()["a"][1], 0.123);
  test_elem(doc.view()["a"][2], -0.123);
  test_elem(doc.view()["ez"], 0.0);
  test_elem(doc.view()["ep"], 3.21);
  test_elem(doc.view()["en"], -3.21);
}

TEST(MongoConversion, Int32) {
  auto test_elem = [](const auto& elem, int32_t value) {
    ASSERT_TRUE(sm::IsOneOf(elem, sm::ElementKind::kNumber));
    EXPECT_FALSE(
        sm::IsOneOf(elem, {sm::ElementKind::kMissing, sm::ElementKind::kNull,
                           sm::ElementKind::kBool, sm::ElementKind::kString,
                           sm::ElementKind::kTimestamp, sm::ElementKind::kArray,
                           sm::ElementKind::kDocument, sm::ElementKind::kOid}));

    EXPECT_DOUBLE_EQ(value, sm::ToDouble(elem));
    EXPECT_EQ(value, sm::ToInt64(elem));
    EXPECT_THROW(sm::ToTimePoint(elem), sm::BadType);
    EXPECT_THROW(sm::ToOid(elem), sm::BadType);
    EXPECT_EQ(std::to_string(value), sm::ToString(elem));
    EXPECT_EQ(!!value, sm::ToBool(elem));
    EXPECT_THROW(sm::ToArray(elem), sm::BadType);
    EXPECT_THROW(sm::ToDocument(elem), sm::BadType);
    EXPECT_THROW(sm::ToStringArray(elem), sm::BadType);
  };

  const auto doc = bsoncxx::from_json(R"({
        "a": [
          { "$numberInt": "0" },
          { "$numberInt": "123" },
          { "$numberInt": "-123" }
        ],
        "ez": { "$numberInt": "0" },
        "ep": { "$numberInt": "321" },
        "en": { "$numberInt": "-321" }
      })");
  test_elem(doc.view()["a"][0], 0);
  test_elem(doc.view()["a"][1], 123);
  test_elem(doc.view()["a"][2], -123);
  test_elem(doc.view()["ez"], 0);
  test_elem(doc.view()["ep"], 321);
  test_elem(doc.view()["en"], -321);
}

TEST(MongoConversion, Int64) {
  auto test_elem = [](const auto& elem, int64_t value) {
    ASSERT_TRUE(sm::IsOneOf(elem, sm::ElementKind::kNumber));
    EXPECT_FALSE(
        sm::IsOneOf(elem, {sm::ElementKind::kMissing, sm::ElementKind::kNull,
                           sm::ElementKind::kBool, sm::ElementKind::kString,
                           sm::ElementKind::kTimestamp, sm::ElementKind::kArray,
                           sm::ElementKind::kDocument, sm::ElementKind::kOid}));

    EXPECT_DOUBLE_EQ(value, sm::ToDouble(elem));
    EXPECT_EQ(value, sm::ToInt64(elem));
    EXPECT_THROW(sm::ToTimePoint(elem), sm::BadType);
    EXPECT_THROW(sm::ToOid(elem), sm::BadType);
    EXPECT_EQ(std::to_string(value), sm::ToString(elem));
    EXPECT_EQ(!!value, sm::ToBool(elem));
    EXPECT_THROW(sm::ToArray(elem), sm::BadType);
    EXPECT_THROW(sm::ToDocument(elem), sm::BadType);
    EXPECT_THROW(sm::ToStringArray(elem), sm::BadType);
  };

  const auto doc = bsoncxx::from_json(R"({
        "a": [
          { "$numberLong": "0" },
          { "$numberLong": "123" },
          { "$numberLong": "-123" }
        ],
        "ez": { "$numberLong": "0" },
        "ep": { "$numberLong": "321" },
        "en": { "$numberLong": "-321" }
      })");
  test_elem(doc.view()["a"][0], 0);
  test_elem(doc.view()["a"][1], 123);
  test_elem(doc.view()["a"][2], -123);
  test_elem(doc.view()["ez"], 0);
  test_elem(doc.view()["ep"], 321);
  test_elem(doc.view()["en"], -321);
}

TEST(MongoConversion, Utf8) {
  auto test_elem = [](const auto& elem, const std::string& value) {
    ASSERT_TRUE(sm::IsOneOf(elem, sm::ElementKind::kString));
    EXPECT_FALSE(
        sm::IsOneOf(elem, {sm::ElementKind::kMissing, sm::ElementKind::kNull,
                           sm::ElementKind::kBool, sm::ElementKind::kNumber,
                           sm::ElementKind::kTimestamp, sm::ElementKind::kArray,
                           sm::ElementKind::kDocument, sm::ElementKind::kOid}));

    EXPECT_THROW(sm::ToDouble(elem), sm::BadType);
    EXPECT_THROW(sm::ToInt64(elem), sm::BadType);
    EXPECT_THROW(sm::ToTimePoint(elem), sm::BadType);
    EXPECT_THROW(sm::ToOid(elem), sm::BadType);
    EXPECT_EQ(value, sm::ToString(elem));
    EXPECT_EQ(!value.empty(), sm::ToBool(elem));
    EXPECT_THROW(sm::ToArray(elem), sm::BadType);
    EXPECT_THROW(sm::ToDocument(elem), sm::BadType);
    EXPECT_THROW(sm::ToStringArray(elem), sm::BadType);
  };

  const auto doc = bsoncxx::from_json(R"({
    "a": ["", "test"],
    "ee": "",
    "e": "string"
  })");
  test_elem(doc.view()["a"][0], {});
  test_elem(doc.view()["a"][1], "test");
  test_elem(doc.view()["ee"], {});
  test_elem(doc.view()["e"], "string");
}

TEST(MongoConversion, Binary) {
  auto test_elem = [](const auto& elem, const std::string& value) {
    ASSERT_TRUE(sm::IsOneOf(elem, sm::ElementKind::kString));
    EXPECT_FALSE(
        sm::IsOneOf(elem, {sm::ElementKind::kMissing, sm::ElementKind::kNull,
                           sm::ElementKind::kBool, sm::ElementKind::kNumber,
                           sm::ElementKind::kTimestamp, sm::ElementKind::kArray,
                           sm::ElementKind::kDocument, sm::ElementKind::kOid}));

    EXPECT_THROW(sm::ToDouble(elem), sm::BadType);
    EXPECT_THROW(sm::ToInt64(elem), sm::BadType);
    EXPECT_THROW(sm::ToTimePoint(elem), sm::BadType);
    EXPECT_THROW(sm::ToOid(elem), sm::BadType);
    EXPECT_EQ(value, sm::ToString(elem));
    EXPECT_EQ(!value.empty(), sm::ToBool(elem));
    EXPECT_THROW(sm::ToArray(elem), sm::BadType);
    EXPECT_THROW(sm::ToDocument(elem), sm::BadType);
    EXPECT_THROW(sm::ToStringArray(elem), sm::BadType);
  };

  const auto doc = bsoncxx::from_json(R"({
    "a": [
      { "$binary": { "base64": "", "subType": "0" } },
      { "$binary": { "base64": "dGVzdA==", "subType": "0" } }
    ],
    "ee": { "$binary": { "base64": "", "subType": "0" } },
    "e": { "$binary": { "base64": "c3RyaW5n", "subType": "0" } }
  })");
  test_elem(doc.view()["a"][0], {});
  test_elem(doc.view()["a"][1], "test");
  test_elem(doc.view()["ee"], {});
  test_elem(doc.view()["e"], "string");
}

TEST(MongoConversion, Date) {
  auto test_elem = [](const auto& elem,
                      const std::chrono::system_clock::time_point& value) {
    ASSERT_TRUE(sm::IsOneOf(elem, sm::ElementKind::kTimestamp));
    EXPECT_FALSE(
        sm::IsOneOf(elem, {sm::ElementKind::kMissing, sm::ElementKind::kNull,
                           sm::ElementKind::kBool, sm::ElementKind::kNumber,
                           sm::ElementKind::kString, sm::ElementKind::kArray,
                           sm::ElementKind::kDocument, sm::ElementKind::kOid}));

    const auto int_value =
        std::chrono::duration_cast<std::chrono::milliseconds>(
            value.time_since_epoch())
            .count();
    EXPECT_DOUBLE_EQ(int_value, sm::ToDouble(elem));
    EXPECT_EQ(int_value, sm::ToInt64(elem));
    EXPECT_EQ(value, sm::ToTimePoint(elem));
    EXPECT_THROW(sm::ToOid(elem), sm::BadType);
    EXPECT_EQ(std::to_string(int_value), sm::ToString(elem));
    EXPECT_TRUE(sm::ToBool(elem));
    EXPECT_THROW(sm::ToArray(elem), sm::BadType);
    EXPECT_THROW(sm::ToDocument(elem), sm::BadType);
    EXPECT_THROW(sm::ToStringArray(elem), sm::BadType);
  };

  const auto doc = bsoncxx::from_json(R"({
    "a": [{ "$date": { "$numberLong": "1535749200000" } }],
    "e": { "$date": { "$numberLong": "1538341200000" } }
  })");
  test_elem(doc.view()["a"][0],
            std::chrono::system_clock::from_time_t(1535749200));
  test_elem(doc.view()["e"],
            std::chrono::system_clock::from_time_t(1538341200));
}

TEST(MongoConversion, Array) {
  auto test_elem = [](const auto& elem,
                      const std::vector<std::string>& strings) {
    ASSERT_TRUE(sm::IsOneOf(elem, sm::ElementKind::kArray));
    EXPECT_FALSE(sm::IsOneOf(
        elem, {sm::ElementKind::kMissing, sm::ElementKind::kNull,
               sm::ElementKind::kBool, sm::ElementKind::kNumber,
               sm::ElementKind::kString, sm::ElementKind::kTimestamp,
               sm::ElementKind::kDocument, sm::ElementKind::kOid}));

    EXPECT_THROW(sm::ToDouble(elem), sm::BadType);
    EXPECT_THROW(sm::ToInt64(elem), sm::BadType);
    EXPECT_THROW(sm::ToTimePoint(elem), sm::BadType);
    EXPECT_THROW(sm::ToOid(elem), sm::BadType);
    EXPECT_THROW(sm::ToString(elem), sm::BadType);
    EXPECT_EQ(!strings.empty(), sm::ToBool(elem));
    EXPECT_THROW(sm::ToDocument(elem), sm::BadType);

    const auto array = sm::ToArray(elem);
    EXPECT_EQ(strings.size(), std::distance(array.begin(), array.end()));
    EXPECT_EQ(strings, sm::ToStringArray(elem));
  };

  const auto doc = bsoncxx::from_json(R"({
    "a": [
      [],
      [1, 2, 3],
      [null, "test", -2.5, { "$numberLong": "1" }]
    ],
    "ee": [],
    "e": ["other", { "$numberInt": "4321" }, false, true]
  })");
  test_elem(doc.view()["a"][0], {});
  test_elem(doc.view()["a"][1], {"1", "2", "3"});
  test_elem(doc.view()["a"][2], {"", "test", std::to_string(-2.5), "1"});
  test_elem(doc.view()["ee"], {});
  test_elem(doc.view()["e"], {"other", "4321", "", "true"});
}

TEST(MongoConversion, Document) {
  auto test_elem = [](const auto& elem, const std::vector<std::string>& keys) {
    ASSERT_TRUE(sm::IsOneOf(elem, sm::ElementKind::kDocument));
    EXPECT_FALSE(sm::IsOneOf(
        elem, {sm::ElementKind::kMissing, sm::ElementKind::kNull,
               sm::ElementKind::kBool, sm::ElementKind::kNumber,
               sm::ElementKind::kString, sm::ElementKind::kTimestamp,
               sm::ElementKind::kArray, sm::ElementKind::kOid}));

    EXPECT_THROW(sm::ToDouble(elem), sm::BadType);
    EXPECT_THROW(sm::ToInt64(elem), sm::BadType);
    EXPECT_THROW(sm::ToTimePoint(elem), sm::BadType);
    EXPECT_THROW(sm::ToOid(elem), sm::BadType);
    EXPECT_THROW(sm::ToString(elem), sm::BadType);
    EXPECT_EQ(!keys.empty(), sm::ToBool(elem));
    EXPECT_THROW(sm::ToArray(elem), sm::BadType);
    EXPECT_THROW(sm::ToStringArray(elem), sm::BadType);

    const auto doc = sm::ToDocument(elem);
    ASSERT_EQ(keys.size(), std::distance(doc.begin(), doc.end()));
    auto doc_it = doc.begin();
    auto keys_it = keys.begin();
    for (; doc_it != doc.end() && keys_it != keys.end(); ++doc_it, ++keys_it) {
      EXPECT_EQ(*keys_it, doc_it->key().to_string());
    }
  };

  const auto doc = bsoncxx::from_json(R"({
    "a": [
      {},
      { "a": 1, "b": 2, "c": 3 }
    ],
    "ee": {},
    "e": { "d": -1, "e": -2, "f": -3 }
  })");
  test_elem(doc.view()["a"][0], {});
  test_elem(doc.view()["a"][1], {"a", "b", "c"});
  test_elem(doc.view()["ee"], {});
  test_elem(doc.view()["e"], {"d", "e", "f"});
}

TEST(MongoConversion, Oid) {
  auto test_elem = [](const auto& elem, const std::string& oid_string) {
    ASSERT_TRUE(sm::IsOneOf(elem, sm::ElementKind::kOid));
    EXPECT_TRUE(sm::IsOneOf(elem, sm::ElementKind::kString));
    EXPECT_FALSE(
        sm::IsOneOf(elem, {sm::ElementKind::kMissing, sm::ElementKind::kNull,
                           sm::ElementKind::kBool, sm::ElementKind::kNumber,
                           sm::ElementKind::kTimestamp, sm::ElementKind::kArray,
                           sm::ElementKind::kDocument}));

    EXPECT_THROW(sm::ToDouble(elem), sm::BadType);
    EXPECT_THROW(sm::ToInt64(elem), sm::BadType);
    EXPECT_THROW(sm::ToTimePoint(elem), sm::BadType);
    EXPECT_EQ(bsoncxx::oid(oid_string), sm::ToOid(elem));
    EXPECT_EQ(oid_string, sm::ToString(elem));
    EXPECT_TRUE(sm::ToBool(elem));
    EXPECT_THROW(sm::ToArray(elem), sm::BadType);
    EXPECT_THROW(sm::ToStringArray(elem), sm::BadType);
  };

  const auto doc = bsoncxx::from_json(R"({
    "a": [{ "$oid": "5b89ac509ecb0a21a6000001" }],
    "e": { "$oid": "5bb139509ecb0a21a6000002" }
  })");
  test_elem(doc.view()["a"][0], "5b89ac509ecb0a21a6000001");
  test_elem(doc.view()["e"], "5bb139509ecb0a21a6000002");
}
