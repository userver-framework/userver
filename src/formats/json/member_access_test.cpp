#include <gtest/gtest.h>

#include <sstream>

#include <formats/json/exception.hpp>
#include <formats/json/serialize.hpp>
#include <formats/json/value_builder.hpp>
#include <formats/json/value_detail.hpp>

namespace {
constexpr const char* kDoc =
    "{\"key1\":1,\"key2\":\"val\",\"key3\":{\"sub\":-1},\"key4\":[1,2,3],"
    "\"key5\":10.5,\"key6\":false}";
}

struct JsonMemberAccess : public ::testing::Test {
  JsonMemberAccess() : js_doc_(formats::json::FromString(kDoc)) {}
  formats::json::Value js_doc_;
};

TEST_F(JsonMemberAccess, ChildBySquareBrakets) {
  EXPECT_EQ(js_doc_["key1"], formats::json::ValueBuilder(1).ExtractValue());
}

TEST_F(JsonMemberAccess, ChildBySquareBraketsTwice) {
  EXPECT_EQ(js_doc_["key3"]["sub"],
            formats::json::ValueBuilder(-1).ExtractValue());
}

TEST_F(JsonMemberAccess, ChildBySquareBraketsMissing) {
  EXPECT_EQ(js_doc_["key_missing"],
            formats::json::ValueBuilder().ExtractValue());
}

TEST_F(JsonMemberAccess, ChildBySquareBraketsWrong) {
  EXPECT_THROW(js_doc_["key1"]["key_wrong"],
               formats::json::TypeMismatchException);
}

TEST_F(JsonMemberAccess, ChildBySquareBraketsArray) {
  EXPECT_EQ(js_doc_["key4"][1], formats::json::ValueBuilder(2).ExtractValue());
}

TEST_F(JsonMemberAccess, ChildBySquareBraketsBounds) {
  EXPECT_THROW(js_doc_["key4"][9], formats::json::OutOfBoundsException);
}

TEST_F(JsonMemberAccess, IterateMemberNames) {
  size_t ind = 1;
  for (auto it = js_doc_.begin(); it != js_doc_.end(); ++it, ++ind) {
    std::ostringstream os("key", std::ios::ate);
    os << ind;
    EXPECT_EQ(it.GetName(), os.str());
    EXPECT_THROW(it.GetIndex(), formats::json::TypeMismatchException);
  }
}

TEST_F(JsonMemberAccess, IterateBothWaysAndCheckValues) {
  auto it = js_doc_.begin();
  EXPECT_EQ(*it, formats::json::ValueBuilder(1).ExtractValue());
  ++it;
  EXPECT_EQ(*it, formats::json::ValueBuilder("val").ExtractValue());
  --it;
  EXPECT_EQ(*it, formats::json::ValueBuilder(1).ExtractValue());
}

TEST_F(JsonMemberAccess, IterateMembersAndCheckKey3) {
  size_t ind = 1;
  for (auto it = js_doc_.begin(); it != js_doc_.end(); ++it, ++ind) {
    if (ind == 3) {
      EXPECT_EQ(*(it->begin()), formats::json::ValueBuilder(-1).ExtractValue());
    }
  }
}

TEST_F(JsonMemberAccess, IterateMembersAndCheckKey4) {
  size_t ind = 1;
  for (auto it = js_doc_.begin(); it != js_doc_.end(); ++it, ++ind) {
    if (ind == 4) {
      EXPECT_THROW((*it)[9], formats::json::OutOfBoundsException);
    }
  }
}

TEST_F(JsonMemberAccess, IterateMembersAndCheckKey4Index) {
  size_t ind = 1;
  for (auto it = js_doc_.begin(); it != js_doc_.end(); ++it, ++ind) {
    if (ind == 4) {
      size_t subind = 0;
      for (auto subit = it->begin(); subit != it->end(); ++subit, ++subind) {
        EXPECT_EQ(subit.GetIndex(), subind);
        EXPECT_THROW(subit.GetName(), formats::json::TypeMismatchException);
      }
    }
  }
}

TEST_F(JsonMemberAccess, CheckPrimitiveTypes) {
  EXPECT_TRUE(js_doc_["key1"].isIntegral());
  EXPECT_TRUE(js_doc_["key1"].isNumeric());
  EXPECT_TRUE(js_doc_["key1"].isUInt64());
  EXPECT_EQ(js_doc_["key1"].asUInt(), 1);

  EXPECT_TRUE(js_doc_["key2"].isString());
  EXPECT_EQ(js_doc_["key2"].asString(), "val");

  EXPECT_TRUE(js_doc_["key3"].isObject());
  EXPECT_TRUE(js_doc_["key3"]["sub"].isIntegral());
  EXPECT_TRUE(js_doc_["key3"]["sub"].isNumeric());
  EXPECT_TRUE(js_doc_["key3"]["sub"].isInt64());
  EXPECT_FALSE(js_doc_["key3"]["sub"].isUInt64());
  EXPECT_EQ(js_doc_["key3"]["sub"].asInt(), -1);

  EXPECT_TRUE(js_doc_["key4"].isArray());
  EXPECT_TRUE(js_doc_["key4"][0].isIntegral());
  EXPECT_TRUE(js_doc_["key4"][0].isNumeric());
  EXPECT_TRUE(js_doc_["key4"][0].isUInt64());
  EXPECT_EQ(js_doc_["key4"][0].asUInt(), 1);

  EXPECT_FALSE(js_doc_["key5"].isIntegral());
  EXPECT_TRUE(js_doc_["key5"].isNumeric());
  EXPECT_TRUE(js_doc_["key5"].isDouble());
  EXPECT_FLOAT_EQ(js_doc_["key5"].asFloat(), 10.5f);
}

TEST_F(JsonMemberAccess, CheckPrimitiveTypeExceptions) {
  EXPECT_THROW(js_doc_["key1"].asBool(), formats::json::TypeMismatchException);
  EXPECT_THROW(js_doc_["key1"].asString(),
               formats::json::TypeMismatchException);
  EXPECT_NO_THROW(js_doc_["key1"].asDouble());

  EXPECT_THROW(js_doc_["key2"].asBool(), formats::json::TypeMismatchException);
  EXPECT_THROW(js_doc_["key2"].asDouble(),
               formats::json::TypeMismatchException);
  EXPECT_THROW(js_doc_["key2"].asUInt(), formats::json::TypeMismatchException);

  EXPECT_THROW(js_doc_["key5"].asBool(), formats::json::TypeMismatchException);
  EXPECT_THROW(js_doc_["key5"].asString(),
               formats::json::TypeMismatchException);
  EXPECT_THROW(js_doc_["key5"].asUInt64(),
               formats::json::TypeMismatchException);
  EXPECT_THROW(js_doc_["key5"].asInt(), formats::json::TypeMismatchException);

  EXPECT_THROW(js_doc_["key6"].asFloat(), formats::json::TypeMismatchException);
  EXPECT_THROW(js_doc_["key6"].asString(),
               formats::json::TypeMismatchException);
  EXPECT_THROW(js_doc_["key6"].asUInt64(),
               formats::json::TypeMismatchException);
  EXPECT_THROW(js_doc_["key6"].asInt(), formats::json::TypeMismatchException);
}

TEST_F(JsonMemberAccess, MemberPaths) {
  // copy/move constructor behaves as if it copied references not values
  formats::json::Value js_copy = js_doc_;
  // check pointer equality of native objects
  EXPECT_EQ(formats::json::detail::GetPtr(js_copy),
            formats::json::detail::GetPtr(js_doc_));

  EXPECT_EQ(js_doc_.GetPath(), "/");
  EXPECT_EQ(js_doc_["key1"].GetPath(), "key1");
  EXPECT_EQ(js_doc_["key3"]["sub"].GetPath(), "key3.sub");
  EXPECT_EQ(js_doc_["key4"][2].GetPath(), "key4.[2]");

  EXPECT_EQ(js_copy.GetPath(), "/");
  EXPECT_EQ(js_copy["key1"].GetPath(), "key1");
  EXPECT_EQ(js_copy["key3"]["sub"].GetPath(), "key3.sub");
  EXPECT_EQ(js_copy["key4"][2].GetPath(), "key4.[2]");
}

TEST_F(JsonMemberAccess, MemberPathsByIterator) {
  auto it = js_doc_.begin();
  it++;
  it++;
  EXPECT_EQ(it->GetPath(), "key3");
  EXPECT_EQ(it->begin()->GetPath(), "key3.sub");
  it++;
  auto arr_it = it->begin();
  EXPECT_EQ((arr_it++)->GetPath(), "key4.[0]");
  EXPECT_EQ((++arr_it)->GetPath(), "key4.[2]");
}

TEST_F(JsonMemberAccess, MemberCount) { EXPECT_EQ(js_doc_.GetSize(), 6); }

TEST_F(JsonMemberAccess, HasMember) {
  EXPECT_TRUE(js_doc_.HasMember("key1"));
  EXPECT_FALSE(js_doc_.HasMember("keyX"));
}

TEST_F(JsonMemberAccess, CopyMoveSubobject) {
  // copy/move constructor behaves as if it copied references from subobjects
  formats::json::Value v(js_doc_["key3"]);

  EXPECT_EQ(v, js_doc_["key3"]);
  // check pointer equality of native objects
  EXPECT_EQ(formats::json::detail::GetPtr(v),
            formats::json::detail::GetPtr(js_doc_["key3"]));
}

TEST_F(JsonMemberAccess, IteratorOnNullThrows) {
  formats::json::Value v;
  EXPECT_THROW(v.begin(), formats::json::TypeMismatchException);
}

TEST_F(JsonMemberAccess, CloneValues) {
  formats::json::Value v = js_doc_.Clone();
  EXPECT_EQ(v, js_doc_);
  // check pointer inequality of native objects
  EXPECT_NE(formats::json::detail::GetPtr(v),
            formats::json::detail::GetPtr(js_doc_));
}

TEST_F(JsonMemberAccess, CreateEmptyAndAccess) {
  formats::json::Value v;
  EXPECT_TRUE(static_cast<formats::json::detail::Value&>(v).IsRoot());
  EXPECT_EQ(v.GetPath(), "/");
  EXPECT_TRUE(v.isNull());
  EXPECT_FALSE(v.HasMember("key_missing"));
  EXPECT_THROW(v.asBool(), formats::json::TypeMismatchException);
}
