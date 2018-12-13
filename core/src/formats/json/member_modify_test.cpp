#include <gtest/gtest.h>

#include <limits>

#include <formats/json/exception.hpp>
#include <formats/json/serialize.hpp>
#include <formats/json/value_builder.hpp>
#include <formats/json/value_detail.hpp>

namespace {
constexpr const char* kDoc =
    "{\"key1\":1,\"key2\":\"val\",\"key3\":{\"sub\":-1},\"key4\":[1,2,3],"
    "\"key5\":10.5}";
}

struct JsonMemberModify : public ::testing::Test {
  JsonMemberModify() : builder_(formats::json::FromString(kDoc)) {}

  formats::json::Value GetValue(formats::json::ValueBuilder& bld) {
    auto v = bld.ExtractValue();
    bld = v;
    return v;
  }

  formats::json::Value GetBuiltValue() { return GetValue(builder_); }

  formats::json::ValueBuilder builder_;
};

TEST_F(JsonMemberModify, BuildNewValueEveryTime) {
  EXPECT_NE(formats::json::detail::GetPtr(GetBuiltValue()),
            formats::json::detail::GetPtr(GetBuiltValue()));
}

TEST_F(JsonMemberModify, CheckPrimitiveTypesChange) {
  builder_["key1"] = -100;
  EXPECT_EQ(GetBuiltValue()["key1"].asInt(), -100);
  builder_["key1"] = "100";
  EXPECT_EQ(GetBuiltValue()["key1"].asString(), "100");
  builder_["key1"] = true;
  EXPECT_TRUE(GetBuiltValue()["key1"].asBool());
}

TEST_F(JsonMemberModify, CheckNestedTypesChange) {
  builder_["key3"]["sub"] = false;
  EXPECT_FALSE(GetBuiltValue()["key3"]["sub"].asBool());
  builder_["key3"] = -100;
  EXPECT_EQ(GetBuiltValue()["key3"].asInt(), -100);
  builder_["key3"] = formats::json::FromString("{\"sub\":-1}");
  EXPECT_EQ(GetBuiltValue()["key3"]["sub"].asInt(), -1);
}

TEST_F(JsonMemberModify, CheckNestedArrayChange) {
  builder_["key4"][1] = 10;
  builder_["key4"][2] = 100;
  EXPECT_EQ(GetBuiltValue()["key4"][0].asInt(), 1);
  EXPECT_EQ(GetBuiltValue()["key4"][1].asInt(), 10);
  EXPECT_EQ(GetBuiltValue()["key4"][2].asInt(), 100);
}

TEST_F(JsonMemberModify, ArrayResize) {
  builder_["key4"].Resize(4);
  EXPECT_EQ(GetBuiltValue()["key4"].GetSize(), 4);

  builder_["key4"][3] = 4;
  for (size_t i = 0; i < 4; ++i) {
    EXPECT_EQ(GetBuiltValue()["key4"][i].asInt(), i + 1);
  }

  builder_["key4"].Resize(1);
  EXPECT_EQ(GetBuiltValue()["key4"].GetSize(), 1);
  EXPECT_EQ(GetBuiltValue()["key4"][0].asInt(), 1);
  EXPECT_THROW(GetBuiltValue()["key4"][2], formats::json::OutOfBoundsException);
}

TEST_F(JsonMemberModify, ArrayFromNull) {
  builder_ = formats::json::ValueBuilder();
  EXPECT_THROW(GetBuiltValue().GetSize(), formats::json::TypeMismatchException);

  builder_.Resize(1);
  EXPECT_EQ(GetBuiltValue().GetSize(), 1);

  builder_[0] = 0;
  EXPECT_EQ(GetBuiltValue()[0].asInt(), 0);
  EXPECT_THROW(GetBuiltValue()[2], formats::json::OutOfBoundsException);
}

TEST_F(JsonMemberModify, ArrayPushBack) {
  builder_ = formats::json::ValueBuilder(formats::json::Type::kArray);

  const auto size = 5;
  for (auto i = 0; i < size; ++i) {
    builder_.PushBack(i);
  }
  EXPECT_EQ(GetBuiltValue().GetSize(), size);

  for (auto i = 0; i < size; ++i) {
    EXPECT_EQ(GetBuiltValue()[i].asInt(), i);
  }
}

TEST_F(JsonMemberModify, PushBackFromExisting) {
  builder_["key4"].PushBack(builder_["key1"]);
  EXPECT_EQ(GetBuiltValue()["key4"].GetSize(), 4);
  EXPECT_EQ(GetBuiltValue()["key4"][3].asInt(), 1);
  EXPECT_EQ(GetBuiltValue()["key1"].asInt(), 1);
}

TEST_F(JsonMemberModify, PushBackWrongTypeThrows) {
  EXPECT_THROW(builder_["key1"].PushBack(1),
               formats::json::TypeMismatchException);
}

TEST_F(JsonMemberModify, ExtractFromSubBuilderThrows) {
  auto bld = builder_["key1"];
  EXPECT_THROW(bld.ExtractValue(), formats::json::JsonException);
}

TEST_F(JsonMemberModify, ObjectIteratorModify) {
  const size_t offset = 3;
  size_t size = 0;
  {
    auto it = builder_.begin();
    for (; it != builder_.end(); ++it) {
      *it = offset + (++size);
    }
  }

  {
    auto it = GetBuiltValue().begin();
    for (size_t i = 1; i <= size; ++i, ++it) {
      std::string name = "key";
      name += std::to_string(i);
      EXPECT_EQ(it.GetName(), name);

      EXPECT_EQ(it->asInt(), i + offset);
    }
  }
}

TEST_F(JsonMemberModify, MemberCount) { EXPECT_EQ(builder_.GetSize(), 5); }

TEST_F(JsonMemberModify, NonArrayThrowGetSize) {
  formats::json::ValueBuilder bld;
  EXPECT_THROW(bld.GetSize(), formats::json::TypeMismatchException);
}

TEST_F(JsonMemberModify, ArrayIteratorRead) {
  builder_ = formats::json::ValueBuilder();
  const auto size = 10;
  builder_.Resize(size);

  for (auto i = 0; i < size; ++i) {
    builder_[i] = i;
  }

  auto it = GetBuiltValue().begin();
  for (auto i = 0; i < size; ++i, ++it) {
    EXPECT_EQ(it->asInt(), i);
  }
  EXPECT_EQ(builder_.GetSize(), size);
}

TEST_F(JsonMemberModify, ArrayIteratorModify) {
  builder_ = formats::json::ValueBuilder();
  const auto size = 10;
  builder_.Resize(size);

  const size_t offset = 3;
  {
    // JsonCpp cannot initialize values of array
    // with iterators (need to first set values with operator[])
    for (auto i = 0; i < size; ++i) {
      builder_[i] = 0;
    }

    auto it = builder_.begin();
    for (auto i = offset; it != builder_.end(); ++it, ++i) {
      *it = i;
    }
  }

  {
    auto it = GetBuiltValue().begin();
    for (auto i = 0; i < size; ++i, ++it) {
      EXPECT_EQ(it->asInt(), i + offset);
    }
  }
  EXPECT_EQ(builder_.GetSize(), size);
}

TEST_F(JsonMemberModify, CreateSpecificType) {
  formats::json::ValueBuilder js_obj(formats::json::Type::kNull);
  EXPECT_THROW(GetValue(js_obj).GetSize(),
               formats::json::TypeMismatchException);

  formats::json::ValueBuilder js_arr(formats::json::Type::kArray);
  EXPECT_THROW(GetValue(js_arr)["key"], formats::json::TypeMismatchException);
}

TEST_F(JsonMemberModify, IteratorOutlivesRoot) {
  auto it = GetBuiltValue().begin();
  {
    formats::json::Value v = GetBuiltValue();
    it = v["key4"].begin();
  }
  EXPECT_EQ((++it)->GetPath(), "key4.[1]");
}

TEST_F(JsonMemberModify, SubdocOutlivesRoot) {
  formats::json::Value v = GetBuiltValue()["key3"];
  EXPECT_TRUE(v.HasMember("sub"));
}

TEST_F(JsonMemberModify, MoveValueBuilder) {
  formats::json::ValueBuilder v = std::move(builder_);
  EXPECT_FALSE(GetValue(v).isNull());
  EXPECT_TRUE(GetBuiltValue().isNull());
}

TEST_F(JsonMemberModify, CheckSubobjectChange) {
  formats::json::ValueBuilder v = GetBuiltValue();
  builder_["key4"] = v["key3"];
  EXPECT_TRUE(GetBuiltValue()["key4"].HasMember("sub"));
  EXPECT_EQ(GetBuiltValue()["key4"]["sub"].asInt(), -1);
  EXPECT_TRUE(GetValue(v)["key3"].HasMember("sub"));
  EXPECT_EQ(GetValue(v)["key3"]["sub"].asInt(), -1);
}

TEST_F(JsonMemberModify, TypeCheckMinMax) {
  formats::json::ValueBuilder bld;
  bld["int8_t"] = std::numeric_limits<int8_t>::min();
  bld["uint8_t"] = std::numeric_limits<uint8_t>::max();
  bld["int16_t"] = std::numeric_limits<int16_t>::min();
  bld["uint16_t"] = std::numeric_limits<uint16_t>::max();
  bld["int32_t"] = std::numeric_limits<int32_t>::min();
  bld["uint32_t"] = std::numeric_limits<uint32_t>::max();
  bld["int64_t"] = std::numeric_limits<int64_t>::min();
  bld["uint64_t"] = std::numeric_limits<uint64_t>::max();
  bld["size_t"] = std::numeric_limits<size_t>::max();
  bld["long"] = std::numeric_limits<long>::min();
  bld["long long"] = std::numeric_limits<long long>::min();

  auto v = GetValue(bld);
  EXPECT_EQ(v["int8_t"].asInt(), std::numeric_limits<int8_t>::min());
  EXPECT_EQ(v["uint8_t"].asUInt(), std::numeric_limits<uint8_t>::max());
  EXPECT_EQ(v["int16_t"].asInt(), std::numeric_limits<int16_t>::min());
  EXPECT_EQ(v["uint16_t"].asUInt(), std::numeric_limits<uint16_t>::max());
  EXPECT_EQ(v["int32_t"].asInt(), std::numeric_limits<int32_t>::min());
  EXPECT_EQ(v["uint32_t"].asUInt(), std::numeric_limits<uint32_t>::max());
  EXPECT_EQ(v["int64_t"].asInt64(), std::numeric_limits<int64_t>::min());
  EXPECT_EQ(v["uint64_t"].asUInt64(), std::numeric_limits<uint64_t>::max());
  EXPECT_EQ(v["size_t"].asUInt64(), std::numeric_limits<size_t>::max());
  EXPECT_EQ(v["long"].asInt64(), std::numeric_limits<long>::min());
  EXPECT_EQ(v["long long"].asInt64(), std::numeric_limits<long long>::min());
}

TEST_F(JsonMemberModify, CannotBuildFromMissing) {
  formats::json::Value v;
  formats::json::ValueBuilder bld;
  EXPECT_THROW(bld = v["missing_key"], formats::json::MemberMissingException);
}
