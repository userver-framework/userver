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
  EXPECT_FALSE(js_doc_["key1"].IsMissing());
  EXPECT_EQ(js_doc_["key1"], formats::json::ValueBuilder(1).ExtractValue());
}

TEST_F(JsonMemberAccess, ChildBySquareBraketsTwice) {
  EXPECT_FALSE(js_doc_["key3"]["sub"].IsMissing());
  EXPECT_EQ(js_doc_["key3"]["sub"],
            formats::json::ValueBuilder(-1).ExtractValue());
}

TEST_F(JsonMemberAccess, ChildBySquareBraketsMissing) {
  EXPECT_NO_THROW(js_doc_["key_missing"]);
  EXPECT_EQ(js_doc_["key_missing"].GetPath(), "key_missing");
  EXPECT_TRUE(js_doc_["key_missing"].IsMissing());
  EXPECT_FALSE(js_doc_["key_missing"].IsNull());
  EXPECT_THROW(js_doc_["key_missing"].As<bool>(),
               formats::json::MemberMissingException);
}

TEST_F(JsonMemberAccess, ChildBySquareBraketsMissingTwice) {
  EXPECT_NO_THROW(js_doc_["key_missing"]["sub"]);
  EXPECT_EQ(js_doc_["key_missing"]["sub"].GetPath(), "key_missing.sub");
  EXPECT_TRUE(js_doc_["key_missing"]["sub"].IsMissing());
  EXPECT_FALSE(js_doc_["key_missing"]["sub"].IsNull());
  EXPECT_THROW(js_doc_["key_missing"]["sub"].As<bool>(),
               formats::json::MemberMissingException);
}

TEST_F(JsonMemberAccess, ChildBySquareBraketsArray) {
  EXPECT_EQ(js_doc_["key4"][1], formats::json::ValueBuilder(2).ExtractValue());
}

TEST_F(JsonMemberAccess, ChildBySquareBraketsBounds) {
  EXPECT_THROW(js_doc_["key4"][9], formats::json::OutOfBoundsException);
}

TEST_F(JsonMemberAccess, IterateMemberNames) {
  EXPECT_TRUE(js_doc_.IsObject());
  size_t ind = 1;
  for (auto it = js_doc_.begin(); it != js_doc_.end(); ++it, ++ind) {
    std::ostringstream os("key", std::ios::ate);
    os << ind;
    EXPECT_EQ(it.GetName(), os.str());
    EXPECT_THROW(it.GetIndex(), formats::json::TypeMismatchException);
  }
}

TEST_F(JsonMemberAccess, IterateAndCheckValues) {
  auto it = js_doc_.begin();
  EXPECT_EQ(*it, formats::json::ValueBuilder(1).ExtractValue());
  ++it;
  EXPECT_EQ(*it, formats::json::ValueBuilder("val").ExtractValue());
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
  EXPECT_TRUE(js_doc_["key1"].IsUInt64());
  EXPECT_EQ(js_doc_["key1"].As<uint64_t>(), 1);

  EXPECT_TRUE(js_doc_["key2"].IsString());
  EXPECT_EQ(js_doc_["key2"].As<std::string>(), "val");

  EXPECT_TRUE(js_doc_["key3"].IsObject());
  EXPECT_TRUE(js_doc_["key3"]["sub"].IsInt64());
  EXPECT_FALSE(js_doc_["key3"]["sub"].IsUInt64());
  EXPECT_EQ(js_doc_["key3"]["sub"].As<int>(), -1);

  EXPECT_TRUE(js_doc_["key4"].IsArray());
  EXPECT_TRUE(js_doc_["key4"][0].IsUInt64());
  EXPECT_EQ(js_doc_["key4"][0].As<uint64_t>(), 1);

  EXPECT_TRUE(js_doc_["key5"].IsDouble());
  EXPECT_DOUBLE_EQ(js_doc_["key5"].As<double>(), 10.5);
}

TEST_F(JsonMemberAccess, CheckPrimitiveTypeExceptions) {
  EXPECT_THROW(js_doc_["key1"].As<bool>(),
               formats::json::TypeMismatchException);
  EXPECT_THROW(js_doc_["key1"].As<std::string>(),
               formats::json::TypeMismatchException);
  EXPECT_NO_THROW(js_doc_["key1"].As<double>());

  EXPECT_THROW(js_doc_["key2"].As<bool>(),
               formats::json::TypeMismatchException);
  EXPECT_THROW(js_doc_["key2"].As<double>(),
               formats::json::TypeMismatchException);
  EXPECT_THROW(js_doc_["key2"].As<uint64_t>(),
               formats::json::TypeMismatchException);

  EXPECT_THROW(js_doc_["key5"].As<bool>(),
               formats::json::TypeMismatchException);
  EXPECT_THROW(js_doc_["key5"].As<std::string>(),
               formats::json::TypeMismatchException);
  EXPECT_THROW(js_doc_["key5"].As<uint64_t>(),
               formats::json::TypeMismatchException);
  EXPECT_THROW(js_doc_["key5"].As<int>(), formats::json::TypeMismatchException);

  EXPECT_THROW(js_doc_["key6"].As<double>(),
               formats::json::TypeMismatchException);
  EXPECT_THROW(js_doc_["key6"].As<std::string>(),
               formats::json::TypeMismatchException);
  EXPECT_THROW(js_doc_["key6"].As<uint64_t>(),
               formats::json::TypeMismatchException);
  EXPECT_THROW(js_doc_["key6"].As<int>(), formats::json::TypeMismatchException);
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
  EXPECT_FALSE(js_doc_["keyX"].HasMember("key1"));
}

TEST_F(JsonMemberAccess, CopyMoveSubobject) {
  // copy/move constructor behaves as if it copied references from subobjects
  formats::json::Value v(js_doc_["key3"]);

  EXPECT_EQ(v, js_doc_["key3"]);
  // check pointer equality of native objects
  EXPECT_EQ(formats::json::detail::GetPtr(v),
            formats::json::detail::GetPtr(js_doc_["key3"]));
}

TEST_F(JsonMemberAccess, IteratorOnNull) {
  formats::json::Value v;
  EXPECT_EQ(v.begin(), v.end());
}

TEST_F(JsonMemberAccess, IteratorOnMissingThrows) {
  formats::json::Value v;
  EXPECT_THROW(v["missing_key"].begin(), formats::json::MemberMissingException);
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
  EXPECT_TRUE(v.IsNull());
  EXPECT_FALSE(v.HasMember("key_missing"));
  EXPECT_THROW(v.As<bool>(), formats::json::TypeMismatchException);
}

TEST_F(JsonMemberAccess, Subfield) {
  formats::json::ValueBuilder body_builder(formats::json::Type::kObject);

  formats::json::Value old = js_doc_["key1"].Clone();
  EXPECT_EQ(old, js_doc_["key1"]);

  body_builder["key1"] = js_doc_["key1"];

  EXPECT_EQ(old, js_doc_["key1"]);
}
