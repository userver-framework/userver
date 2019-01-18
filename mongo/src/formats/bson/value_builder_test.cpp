#include <gtest/gtest.h>

#include <chrono>

#include <formats/bson.hpp>

namespace fb = formats::bson;

TEST(BsonValueBuilder, Default) {
  fb::ValueBuilder b;
  EXPECT_EQ(0, b.GetSize());

  auto val = b.ExtractValue();
  EXPECT_TRUE(val.IsNull());
}

TEST(BsonValueBuilder, Null) {
  auto val = fb::ValueBuilder(nullptr).ExtractValue();
  EXPECT_TRUE(val.IsNull());
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

TEST(BsonValueBuilder, Array) {
  fb::ValueBuilder builder;
  EXPECT_EQ(0, builder.GetSize());

  builder.PushBack(123);
  ASSERT_EQ(1, builder.GetSize());
  {
    auto val = fb::ValueBuilder(builder).ExtractValue();
    ASSERT_EQ(1, val.GetSize());
    ASSERT_TRUE(val[0].IsInt32());
    EXPECT_EQ(123, val[0].As<int>());
  }

  ASSERT_EQ(1, builder.GetSize());
  builder[0] = "str";
  {
    auto val = fb::ValueBuilder(builder).ExtractValue();
    ASSERT_EQ(1, val.GetSize());
    ASSERT_TRUE(val[0].IsString());
    EXPECT_EQ("str", val[0].As<std::string>());
  }

  ASSERT_EQ(1, builder.GetSize());
  builder.Resize(2);
  builder[1]["test"] = 0.5;
  {
    auto val = fb::ValueBuilder(builder).ExtractValue();
    ASSERT_EQ(2, val.GetSize());
    ASSERT_TRUE(val[0].IsString());
    EXPECT_EQ("str", val[0].As<std::string>());
    ASSERT_TRUE(val[1].IsDocument());
    ASSERT_TRUE(val[1]["test"].IsDouble());
    EXPECT_DOUBLE_EQ(0.5, val[1]["test"].As<double>());
  }

  ASSERT_EQ(2, builder.GetSize());
  for (auto it = builder.begin(); it != builder.end(); ++it) {
    *it = size_t{it.GetIndex()};
  }
  {
    auto val = fb::ValueBuilder(builder).ExtractValue();
    ASSERT_EQ(2, val.GetSize());
    for (auto it = val.begin(); it != val.end(); ++it) {
      ASSERT_TRUE(it->IsInt64());
      EXPECT_EQ(it.GetIndex(), it->As<size_t>());
    }
  }

  ASSERT_EQ(2, builder.GetSize());
  builder.Resize(0);
  auto val = builder.ExtractValue();
  ASSERT_TRUE(val.IsArray());
  ASSERT_EQ(0, val.GetSize());
}

TEST(BsonValueBuilder, Document) {
  fb::ValueBuilder builder;
  EXPECT_EQ(0, builder.GetSize());

  builder["first"] = 123;
  ASSERT_EQ(1, builder.GetSize());
  {
    auto val = fb::ValueBuilder(builder).ExtractValue();
    EXPECT_EQ(1, val.GetSize());
    ASSERT_TRUE(val["first"].IsInt32());
    EXPECT_EQ(123, val["first"].As<int>());
  }

  ASSERT_EQ(1, builder.GetSize());
  builder["first"] = "str";
  {
    auto val = fb::ValueBuilder(builder).ExtractValue();
    EXPECT_EQ(1, val.GetSize());
    ASSERT_TRUE(val["first"].IsString());
    EXPECT_EQ("str", val["first"].As<std::string>());
  }

  ASSERT_EQ(1, builder.GetSize());
  builder["second"]["test"] = 0.5;
  {
    auto val = fb::ValueBuilder(builder).ExtractValue();
    EXPECT_EQ(2, val.GetSize());
    ASSERT_TRUE(val["first"].IsString());
    EXPECT_EQ("str", val["first"].As<std::string>());
    ASSERT_TRUE(val["second"].IsDocument());
    ASSERT_TRUE(val["second"]["test"].IsDouble());
    EXPECT_DOUBLE_EQ(0.5, val["second"]["test"].As<double>());
  }

  ASSERT_EQ(2, builder.GetSize());
  for (auto it = builder.begin(); it != builder.end(); ++it) {
    *it = it.GetName();
  }
  {
    auto val = fb::ValueBuilder(builder).ExtractValue();
    EXPECT_EQ(2, val.GetSize());
    for (auto it = val.begin(); it != val.end(); ++it) {
      ASSERT_TRUE(it->IsString());
      EXPECT_EQ(it.GetName(), it->As<std::string>());
    }
  }

  ASSERT_THROW(builder.Resize(0), fb::TypeMismatchException);
}

TEST(BsonValueBuilder, MutateDocument) {
  static const auto kDoc =
      fb::MakeDoc("arr", fb::MakeArray(1, "elem", fb::MinKey{}),  //
                  "doc", fb::MakeDoc("b", true, "i", 0, "d", -1.25));

  fb::ValueBuilder builder(kDoc);
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
  EXPECT_EQ(2, val.GetSize());

  ASSERT_TRUE(val["arr"].IsArray());
  ASSERT_EQ(4, val["arr"].GetSize());

  ASSERT_TRUE(val["arr"][0].IsInt32());
  EXPECT_EQ(0, val["arr"][0].As<int>());

  ASSERT_TRUE(val["arr"][1].IsMinKey());

  ASSERT_TRUE(val["arr"][2].IsNull());

  ASSERT_TRUE(val["arr"][3].IsString());
  EXPECT_EQ("last", val["arr"][3].As<std::string>());

  ASSERT_TRUE(val["doc"].IsDocument());
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
  ASSERT_EQ(1, val["doc"]["a"].GetSize());
  ASSERT_TRUE(val["doc"]["a"][0].As<bool>());

  ASSERT_TRUE(val["doc"]["aa"].IsArray());
  ASSERT_EQ(1, val["doc"]["aa"].GetSize());
  ASSERT_FALSE(val["doc"]["aa"][0].As<bool>());
}

TEST(BsonValueBuilder, UnsetFieldsAreSkipped) {
  fb::ValueBuilder builder;
  builder["set"] = 1;
  builder["unset"];
  auto val = builder.ExtractValue();
  EXPECT_EQ(1, val.GetSize());
  EXPECT_TRUE(val.HasMember("set"));
  EXPECT_FALSE(val.HasMember("unset"));
}
