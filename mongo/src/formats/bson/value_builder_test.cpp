#include <gtest/gtest.h>

#include <chrono>
#include <string>
#include <type_traits>
#include <unordered_map>
#include <vector>

#include <formats/common/value_builder_test.hpp>
#include <userver/formats/bson.hpp>
#include <userver/formats/serialize/common_containers.hpp>
#include <userver/utest/assert_macros.hpp>

USERVER_NAMESPACE_BEGIN

namespace fb = formats::bson;

static_assert(
    std::is_assignable_v<decltype(*std::declval<fb::ValueBuilder>().begin()),
                         fb::ValueBuilder>,
    "ValueBuilder iterators are assignable");

TEST(BsonValueBuilder, Default) {
  fb::ValueBuilder b;
  EXPECT_TRUE(b.IsEmpty());
  EXPECT_EQ(0, b.GetSize());

  auto val = b.ExtractValue();
  EXPECT_TRUE(val.IsNull());
}

TEST(BsonValueBuilder, Null) {
  auto val = fb::ValueBuilder(nullptr).ExtractValue();
  EXPECT_TRUE(val.IsNull());
  EXPECT_EQ(val, fb::ValueBuilder(fb::kNull).ExtractValue());
  EXPECT_EQ(val,
            fb::ValueBuilder(fb::ValueBuilder::Type::kNull).ExtractValue());
}

TEST(BsonValueBuilder, Bool) {
  auto val = fb::ValueBuilder(true).ExtractValue();
  ASSERT_TRUE(val.IsBool());
  EXPECT_TRUE(val.As<bool>());
}

TEST(BsonValueBuilder, Int32) {
  auto val = fb::ValueBuilder(123).ExtractValue();
  ASSERT_TRUE(val.IsInt32());
  EXPECT_EQ(123, val.As<int>());
}

TEST(BsonValueBuilder, Int64) {
  auto val = fb::ValueBuilder(int64_t{123}).ExtractValue();
  EXPECT_FALSE(val.IsInt32());
  ASSERT_TRUE(val.IsInt64());
  EXPECT_EQ(123, val.As<int64_t>());
}

TEST(BsonValueBuilder, Uint64) {
  auto val = fb::ValueBuilder(uint64_t{123}).ExtractValue();
  EXPECT_FALSE(val.IsInt32());
  ASSERT_TRUE(val.IsInt64());
  EXPECT_EQ(123, val.As<uint64_t>());

  EXPECT_ANY_THROW(fb::ValueBuilder(static_cast<uint64_t>(-1)));
}

TEST(BsonValueBuilder, Double) {
  auto val = fb::ValueBuilder(2.5).ExtractValue();
  ASSERT_TRUE(val.IsDouble());
  EXPECT_DOUBLE_EQ(2.5, val.As<double>());
}

TEST(BsonValueBuilder, String) {
  auto val1 = fb::ValueBuilder("test").ExtractValue();
  auto val2 = fb::ValueBuilder(std::string("test")).ExtractValue();
  ASSERT_TRUE(val1.IsString());
  EXPECT_EQ("test", val1.As<std::string>());
  EXPECT_EQ(val1, val2);
}

TEST(BsonValueBuilder, TimePoint) {
  auto now = std::chrono::system_clock::now();
  auto val = fb::ValueBuilder(now).ExtractValue();
  ASSERT_TRUE(val.IsDateTime());
  EXPECT_LE(now - val.As<decltype(now)>(), std::chrono::milliseconds(1));
}

TEST(BsonValueBuilder, Oid) {
  auto oid = fb::Oid("5b89ac509ecb0a21a6000001");
  auto val = fb::ValueBuilder(oid).ExtractValue();
  ASSERT_TRUE(val.IsOid());
  EXPECT_EQ(oid, val.As<fb::Oid>());

  const auto tp_before =
      std::chrono::system_clock::now() - std::chrono::minutes{1};
  auto new_val1 = fb::ValueBuilder(fb::Oid{}).ExtractValue();
  auto new_val2 = fb::ValueBuilder(fb::Oid{}).ExtractValue();
  const auto tp_after =
      std::chrono::system_clock::now() + std::chrono::minutes{1};
  ASSERT_TRUE(new_val1.IsOid());
  ASSERT_TRUE(new_val2.IsOid());
  EXPECT_NE(new_val1, new_val2);
  EXPECT_GT(new_val1.As<fb::Oid>().GetTimePoint(), tp_before);
  EXPECT_GT(new_val2.As<fb::Oid>().GetTimePoint(), tp_before);
  EXPECT_LT(new_val1.As<fb::Oid>().GetTimePoint(), tp_after);
  EXPECT_LT(new_val2.As<fb::Oid>().GetTimePoint(), tp_after);
}

TEST(BsonValueBuilder, OidMakeMinimalFor) {
  const auto now = std::chrono::system_clock::now();
  auto oid = fb::Oid::MakeMinimalFor(now);
  EXPECT_EQ(oid.GetTimestamp(), std::chrono::system_clock::to_time_t(now));
  EXPECT_EQ(oid, fb::Oid::MakeMinimalFor(now));

  auto val = fb::ValueBuilder(oid).ExtractValue();
  ASSERT_TRUE(val.IsOid());
  EXPECT_EQ(oid, val.As<fb::Oid>());
}

TEST(BsonValueBuilder, Binary) {
  auto binary = fb::Binary("test");
  auto val = fb::ValueBuilder(binary).ExtractValue();
  ASSERT_TRUE(val.IsBinary());
  EXPECT_EQ(binary, val.As<fb::Binary>());
}

TEST(BsonValueBuilder, Decimal128) {
  auto decimal = fb::Decimal128("123123.123");
  auto val = fb::ValueBuilder(decimal).ExtractValue();
  ASSERT_TRUE(val.IsDecimal128());
  EXPECT_EQ(decimal, val.As<fb::Decimal128>());
}

TEST(BsonValueBuilder, MinMaxKey) {
  auto minkey = fb::ValueBuilder(fb::MinKey{}).ExtractValue();
  auto maxkey = fb::ValueBuilder(fb::MaxKey{}).ExtractValue();
  EXPECT_TRUE(minkey.IsMinKey());
  EXPECT_TRUE(maxkey.IsMaxKey());
}

TEST(BsonValueBuilder, Timestamp) {
  const auto ts_before1 = fb::Timestamp(1111111111, 12321);
  const auto ts_before2 = fb::Timestamp(1554138241, 10000);
  const auto ts_after1 = fb::Timestamp(1999999999, 12321);
  const auto ts_after2 = fb::Timestamp(1554138241, 99999);

  auto val = fb::ValueBuilder(fb::Timestamp(1554138241, 12321)).ExtractValue();
  ASSERT_TRUE(val.IsTimestamp());
  auto ts = val.As<fb::Timestamp>();
  EXPECT_EQ(1554138241, ts.GetTimestamp());
  EXPECT_EQ(12321, ts.GetIncrement());
  EXPECT_EQ(0x5CA2448100003021, ts.Pack());
  EXPECT_EQ(ts, fb::Timestamp::Unpack(ts.Pack()));

  EXPECT_NE(ts, ts_before1);
  EXPECT_GT(ts, ts_before1);
  EXPECT_NE(ts, ts_before2);
  EXPECT_GT(ts, ts_before2);
  EXPECT_NE(ts, ts_after1);
  EXPECT_LT(ts, ts_after1);
  EXPECT_NE(ts, ts_after2);
  EXPECT_LT(ts, ts_after2);
}

TEST(BsonValueBuilder, Array) {
  fb::ValueBuilder builder;
  EXPECT_TRUE(builder.IsEmpty());
  EXPECT_EQ(0, builder.GetSize());

  builder.PushBack(123);
  EXPECT_FALSE(builder.IsEmpty());
  ASSERT_EQ(1, builder.GetSize());
  {
    auto val = fb::ValueBuilder(builder).ExtractValue();
    EXPECT_FALSE(val.IsEmpty());
    ASSERT_EQ(1, val.GetSize());
    ASSERT_TRUE(val[0].IsInt32());
    EXPECT_EQ(123, val[0].As<int>());
  }

  EXPECT_FALSE(builder.IsEmpty());
  ASSERT_EQ(1, builder.GetSize());
  builder[0] = "str";
  {
    auto val = fb::ValueBuilder(builder).ExtractValue();
    EXPECT_FALSE(val.IsEmpty());
    ASSERT_EQ(1, val.GetSize());
    ASSERT_TRUE(val[0].IsString());
    EXPECT_EQ("str", val[0].As<std::string>());
  }

  EXPECT_FALSE(builder.IsEmpty());
  ASSERT_EQ(1, builder.GetSize());
  builder.Resize(2);
  builder[1]["test"] = 0.5;
  {
    auto val = fb::ValueBuilder(builder).ExtractValue();
    EXPECT_FALSE(val.IsEmpty());
    ASSERT_EQ(2, val.GetSize());
    ASSERT_TRUE(val[0].IsString());
    EXPECT_EQ("str", val[0].As<std::string>());
    ASSERT_TRUE(val[1].IsDocument());
    ASSERT_TRUE(val[1]["test"].IsDouble());
    EXPECT_DOUBLE_EQ(0.5, val[1]["test"].As<double>());
  }

  EXPECT_FALSE(builder.IsEmpty());
  ASSERT_EQ(2, builder.GetSize());
  for (auto it = builder.begin(); it != builder.end(); ++it) {
    *it = size_t{it.GetIndex()};
  }
  {
    auto val = fb::ValueBuilder(builder).ExtractValue();
    EXPECT_FALSE(val.IsEmpty());
    ASSERT_EQ(2, val.GetSize());
    for (auto it = val.begin(); it != val.end(); ++it) {
      ASSERT_TRUE(it->IsInt64());
      EXPECT_EQ(it.GetIndex(), it->As<size_t>());
    }
  }

  EXPECT_FALSE(builder.IsEmpty());
  ASSERT_EQ(2, builder.GetSize());
  builder.Resize(0);
  auto val = builder.ExtractValue();
  ASSERT_TRUE(val.IsArray());
  EXPECT_TRUE(val.IsEmpty());
  ASSERT_EQ(0, val.GetSize());
}

TEST(BsonValueBuilder, Document) {
  fb::ValueBuilder builder;
  EXPECT_TRUE(builder.IsEmpty());
  EXPECT_EQ(0, builder.GetSize());

  builder["first"] = 123;
  EXPECT_FALSE(builder.IsEmpty());
  ASSERT_EQ(1, builder.GetSize());
  {
    auto val = fb::ValueBuilder(builder).ExtractValue();
    EXPECT_FALSE(val.IsEmpty());
    EXPECT_EQ(1, val.GetSize());
    ASSERT_TRUE(val["first"].IsInt32());
    EXPECT_EQ(123, val["first"].As<int>());
  }

  EXPECT_FALSE(builder.IsEmpty());
  ASSERT_EQ(1, builder.GetSize());
  builder["first"] = "str";
  {
    auto val = fb::ValueBuilder(builder).ExtractValue();
    EXPECT_FALSE(val.IsEmpty());
    EXPECT_EQ(1, val.GetSize());
    ASSERT_TRUE(val["first"].IsString());
    EXPECT_EQ("str", val["first"].As<std::string>());
  }

  EXPECT_FALSE(builder.IsEmpty());
  ASSERT_EQ(1, builder.GetSize());
  builder["second"]["test"] = 0.5;
  {
    auto val = fb::ValueBuilder(builder).ExtractValue();
    EXPECT_FALSE(val.IsEmpty());
    EXPECT_EQ(2, val.GetSize());
    ASSERT_TRUE(val["first"].IsString());
    EXPECT_EQ("str", val["first"].As<std::string>());
    ASSERT_TRUE(val["second"].IsDocument());
    ASSERT_TRUE(val["second"]["test"].IsDouble());
    EXPECT_DOUBLE_EQ(0.5, val["second"]["test"].As<double>());
  }

  EXPECT_FALSE(builder.IsEmpty());
  ASSERT_EQ(2, builder.GetSize());
  for (auto it = builder.begin(); it != builder.end(); ++it) {
    *it = it.GetName();
  }
  {
    auto val = fb::ValueBuilder(builder).ExtractValue();
    EXPECT_FALSE(val.IsEmpty());
    EXPECT_EQ(2, val.GetSize());
    for (auto it = val.begin(); it != val.end(); ++it) {
      ASSERT_TRUE(it->IsString());
      EXPECT_EQ(it.GetName(), it->As<std::string>());
    }
  }

  UASSERT_THROW(builder.Resize(0), fb::TypeMismatchException);
}

TEST(BsonValueBuilder, MutateDocument) {
  static const auto kDoc =
      fb::MakeDoc("arr", fb::MakeArray(1, "elem", fb::MinKey{}),  //
                  "doc", fb::MakeDoc("b", true, "i", 0, "d", -1.25));

  fb::ValueBuilder builder(kDoc);
  EXPECT_FALSE(builder.IsEmpty());
  EXPECT_EQ(2, builder.GetSize());

  builder["arr"][0] = builder["doc"]["i"];
  builder["arr"][1] = builder["arr"][2];
  builder["arr"].PushBack("last");
  builder["arr"][2] = nullptr;

  builder["doc"]["d"] = builder["doc"];
  builder["doc"]["i"] = -1;
  builder["doc"]["u"] = builder["doc"]["i"];
  builder["doc"]["u"] = 2;
  builder["doc"]["a"].PushBack(true);
  builder["doc"]["aa"] = builder["doc"]["a"];
  builder["doc"]["a"] = builder["doc"]["aa"];
  builder["doc"]["aa"][0] = false;

  auto val = builder.ExtractValue();
  EXPECT_FALSE(val.IsEmpty());
  EXPECT_EQ(2, val.GetSize());

  ASSERT_TRUE(val["arr"].IsArray());
  EXPECT_FALSE(val["arr"].IsEmpty());
  ASSERT_EQ(4, val["arr"].GetSize());

  ASSERT_TRUE(val["arr"][0].IsInt32());
  EXPECT_EQ(0, val["arr"][0].As<int>());

  ASSERT_TRUE(val["arr"][1].IsMinKey());

  ASSERT_TRUE(val["arr"][2].IsNull());

  ASSERT_TRUE(val["arr"][3].IsString());
  EXPECT_EQ("last", val["arr"][3].As<std::string>());

  ASSERT_TRUE(val["doc"].IsDocument());
  EXPECT_FALSE(val["doc"].IsEmpty());
  EXPECT_EQ(6, val["doc"].GetSize());

  ASSERT_TRUE(val["doc"]["b"].IsBool());
  EXPECT_TRUE(val["doc"]["b"].As<bool>());

  ASSERT_TRUE(val["doc"]["d"].IsDocument());
  ASSERT_NE(kDoc["doc"], val["doc"]);
  ASSERT_EQ(kDoc["doc"], val["doc"]["d"]);

  ASSERT_TRUE(val["doc"]["i"].IsInt32());
  EXPECT_EQ(-1, val["doc"]["i"].As<int>());

  ASSERT_TRUE(val["doc"]["u"].IsInt32());
  EXPECT_EQ(2, val["doc"]["u"].As<int>());

  ASSERT_TRUE(val["doc"]["a"].IsArray());
  EXPECT_FALSE(val["doc"]["a"].IsEmpty());
  ASSERT_EQ(1, val["doc"]["a"].GetSize());
  ASSERT_TRUE(val["doc"]["a"][0].As<bool>());

  ASSERT_TRUE(val["doc"]["aa"].IsArray());
  EXPECT_FALSE(val["doc"]["aa"].IsEmpty());
  ASSERT_EQ(1, val["doc"]["aa"].GetSize());
  ASSERT_FALSE(val["doc"]["aa"][0].As<bool>());
}

TEST(BsonValueBuilder, UnsetFieldsAreSkipped) {
  fb::ValueBuilder builder;
  builder["set"] = 1;
  builder["unset"];
  auto val = builder.ExtractValue();
  EXPECT_FALSE(val.IsEmpty());
  EXPECT_EQ(1, val.GetSize());
  EXPECT_TRUE(val.HasMember("set"));
  EXPECT_FALSE(val.HasMember("unset"));
}

TEST(BsonValueBuilder, PredefType) {
  {
    fb::ValueBuilder doc_builder(fb::ValueBuilder::Type::kObject);
    EXPECT_TRUE(doc_builder.IsEmpty());
    EXPECT_EQ(0, doc_builder.GetSize());
    UEXPECT_THROW(doc_builder.Resize(0), fb::TypeMismatchException);
    UEXPECT_NO_THROW(doc_builder["a"] = 1);
    UEXPECT_NO_THROW(fb::Document(doc_builder.ExtractValue()));
  }

  {
    fb::ValueBuilder arr_builder(fb::ValueBuilder::Type::kArray);
    EXPECT_TRUE(arr_builder.IsEmpty());
    EXPECT_EQ(0, arr_builder.GetSize());
    UEXPECT_THROW(arr_builder["a"], fb::TypeMismatchException);
    UEXPECT_NO_THROW(arr_builder.PushBack(1));
    UEXPECT_THROW(fb::Document(arr_builder.ExtractValue()),
                  fb::TypeMismatchException);
  }
}

TEST(BsonValueBuilder, Serialize) {
  const std::vector<int> test_vector{1, 3, 2, 4};
  const std::unordered_map<std::string, int> test_map{{"one", 1}, {"two", 2}};

  fb::ValueBuilder builder;
  builder["arr"] = test_vector;
  builder["doc"] = test_map;
  const auto value = builder.ExtractValue();

  ASSERT_TRUE(value["arr"].IsArray());
  EXPECT_EQ(test_vector.empty(), value["arr"].IsEmpty());
  ASSERT_EQ(test_vector.size(), value["arr"].GetSize());
  for (size_t i = 0; i < test_vector.size(); ++i) {
    EXPECT_EQ(test_vector[i], value["arr"][i].As<int>());
  }

  ASSERT_TRUE(value["doc"].IsObject());
  EXPECT_FALSE(value["doc"].IsEmpty());
  EXPECT_EQ(2, value["doc"].GetSize());
  EXPECT_EQ(1, value["doc"]["one"].As<int>(-1));
  EXPECT_EQ(2, value["doc"]["two"].As<int>(-1));
}

template <>
struct InstantiationDeathTest<formats::bson::ValueBuilder>
    : public ::testing::Test {
  using ValueBuilder = formats::bson::ValueBuilder;

  using Exception = formats::bson::BsonException;
};

INSTANTIATE_TYPED_TEST_SUITE_P(FormatsBson, InstantiationDeathTest,
                               formats::bson::ValueBuilder);
INSTANTIATE_TYPED_TEST_SUITE_P(FormatsBson, CommonValueBuilderTests,
                               formats::bson::ValueBuilder);

TEST(BsonValueBuilder, ExampleUsage) {
  /// [Sample formats::bson::ValueBuilder usage]
  // #include <userver/formats/bson.hpp>
  formats::bson::ValueBuilder builder;
  builder["key1"] = 1;
  builder["key2"]["key3"] = "val";
  formats::bson::Value bson = builder.ExtractValue();

  ASSERT_EQ(bson["key1"].As<int>(), 1);
  ASSERT_EQ(bson["key2"]["key3"].As<std::string>(), "val");
  /// [Sample formats::bson::ValueBuilder usage]
}

/// [Sample Customization formats::bson::ValueBuilder usage]
namespace my_namespace {

struct MyKeyValue {
  std::string field1;
  int field2;
};

// The function must be declared in the namespace of your type
formats::bson::Value Serialize(const MyKeyValue& data,
                               formats::serialize::To<formats::bson::Value>) {
  formats::bson::ValueBuilder builder;
  builder["field1"] = data.field1;
  builder["field2"] = data.field2;

  return builder.ExtractValue();
}

TEST(BsonValueBuilder, ExampleCustomization) {
  MyKeyValue object = {"val", 1};
  formats::bson::ValueBuilder builder;
  builder["example"] = object;
  auto bson = builder.ExtractValue();
  ASSERT_EQ(bson["example"]["field1"].As<std::string>(), "val");
  ASSERT_EQ(bson["example"]["field2"].As<int>(), 1);
}

}  // namespace my_namespace

/// [Sample Customization formats::bson::ValueBuilder usage]

USERVER_NAMESPACE_END
