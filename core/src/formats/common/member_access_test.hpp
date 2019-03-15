#include <gtest/gtest-typed-test.h>

#include <sstream>

template <class T>
struct MemberAccess : public ::testing::Test {};
TYPED_TEST_CASE_P(MemberAccess);

TYPED_TEST_P(MemberAccess, ChildBySquareBrakets) {
  using ValueBuilder = typename TestFixture::ValueBuilder;
  using Value = typename TestFixture::Value;

  EXPECT_FALSE(this->doc_["key1"].IsMissing());
  EXPECT_EQ(this->doc_["key1"], ValueBuilder(1).ExtractValue());
}

TYPED_TEST_P(MemberAccess, ChildBySquareBraketsTwice) {
  using ValueBuilder = typename TestFixture::ValueBuilder;
  using Value = typename TestFixture::Value;

  EXPECT_FALSE(this->doc_["key3"]["sub"].IsMissing());
  EXPECT_EQ(this->doc_["key3"]["sub"], ValueBuilder(-1).ExtractValue());
}

TYPED_TEST_P(MemberAccess, ChildBySquareBraketsMissing) {
  using ValueBuilder = typename TestFixture::ValueBuilder;
  using Value = typename TestFixture::Value;
  using MemberMissingException = typename TestFixture::MemberMissingException;

  EXPECT_NO_THROW(this->doc_["key_missing"]);
  EXPECT_EQ(this->doc_["key_missing"].GetPath(), "key_missing");
  EXPECT_TRUE(this->doc_["key_missing"].IsMissing());
  EXPECT_FALSE(this->doc_["key_missing"].IsNull());
  EXPECT_THROW(this->doc_["key_missing"].template As<bool>(),
               MemberMissingException);
}

TYPED_TEST_P(MemberAccess, ChildBySquareBraketsMissingTwice) {
  using MemberMissingException = typename TestFixture::MemberMissingException;

  EXPECT_NO_THROW(this->doc_["key_missing"]["sub"]);
  EXPECT_EQ(this->doc_["key_missing"]["sub"].GetPath(), "key_missing.sub");
  EXPECT_TRUE(this->doc_["key_missing"]["sub"].IsMissing());
  EXPECT_FALSE(this->doc_["key_missing"]["sub"].IsNull());
  EXPECT_THROW(this->doc_["key_missing"]["sub"].template As<bool>(),
               MemberMissingException);
}

TYPED_TEST_P(MemberAccess, ChildBySquareBraketsArray) {
  using ValueBuilder = typename TestFixture::ValueBuilder;

  EXPECT_EQ(this->doc_["key4"][1], ValueBuilder(2).ExtractValue());
}

TYPED_TEST_P(MemberAccess, ChildBySquareBraketsBounds) {
  using OutOfBoundsException = typename TestFixture::OutOfBoundsException;

  EXPECT_THROW(this->doc_["key4"][9], OutOfBoundsException);
}

TYPED_TEST_P(MemberAccess, IterateMemberNames) {
  using TypeMismatchException = typename TestFixture::TypeMismatchException;

  EXPECT_TRUE(this->doc_.IsObject());
  size_t ind = 1;
  for (auto it = this->doc_.begin(); it != this->doc_.end(); ++it, ++ind) {
    std::ostringstream os("key", std::ios::ate);
    os << ind;
    EXPECT_EQ(it.GetName(), os.str()) << "Failed for index " << ind;
    EXPECT_THROW(it.GetIndex(), TypeMismatchException)
        << "Failed for index " << ind;
  }
}

TYPED_TEST_P(MemberAccess, IterateAndCheckValues) {
  using ValueBuilder = typename TestFixture::ValueBuilder;

  auto it = this->doc_.begin();
  EXPECT_EQ(*it, ValueBuilder(1).ExtractValue());
  ++it;
  EXPECT_EQ(*it, ValueBuilder("val").ExtractValue());
}

TYPED_TEST_P(MemberAccess, IterateMembersAndCheckKey3) {
  using ValueBuilder = typename TestFixture::ValueBuilder;

  size_t ind = 1;
  for (auto it = this->doc_.begin(); it != this->doc_.end(); ++it, ++ind) {
    if (ind == 3) {
      EXPECT_EQ(*(it->begin()), ValueBuilder(-1).ExtractValue());
    }
  }
}

TYPED_TEST_P(MemberAccess, IterateMembersAndCheckKey4) {
  using OutOfBoundsException = typename TestFixture::OutOfBoundsException;

  size_t ind = 1;
  for (auto it = this->doc_.begin(); it != this->doc_.end(); ++it, ++ind) {
    if (ind == 4) {
      EXPECT_THROW((*it)[9], OutOfBoundsException);
    }
  }
}

TYPED_TEST_P(MemberAccess, IterateMembersAndCheckKey4Index) {
  using TypeMismatchException = typename TestFixture::TypeMismatchException;

  size_t ind = 1;
  for (auto it = this->doc_.begin(); it != this->doc_.end(); ++it, ++ind) {
    if (ind == 4) {
      size_t subind = 0;
      for (auto subit = it->begin(); subit != it->end(); ++subit, ++subind) {
        EXPECT_EQ(subit.GetIndex(), subind);
        EXPECT_THROW(subit.GetName(), TypeMismatchException);
      }
    }
  }
}

TYPED_TEST_P(MemberAccess, CheckPrimitiveTypes) {
  EXPECT_TRUE(this->doc_["key1"].IsUInt64());
  EXPECT_EQ(this->doc_["key1"].template As<uint64_t>(), 1);

  EXPECT_TRUE(this->doc_["key2"].IsString());
  EXPECT_EQ(this->doc_["key2"].template As<std::string>(), "val");

  EXPECT_TRUE(this->doc_["key3"].IsObject());
  EXPECT_TRUE(this->doc_["key3"]["sub"].IsInt64());
  EXPECT_EQ(this->doc_["key3"]["sub"].template As<int>(), -1);

  EXPECT_TRUE(this->doc_["key4"].IsArray());
  EXPECT_TRUE(this->doc_["key4"][0].IsUInt64());
  EXPECT_EQ(this->doc_["key4"][0].template As<uint64_t>(), 1);

  EXPECT_TRUE(this->doc_["key5"].IsDouble());
  EXPECT_DOUBLE_EQ(this->doc_["key5"].template As<double>(), 10.5);
}

TYPED_TEST_P(MemberAccess, CheckPrimitiveTypeExceptions) {
  using TypeMismatchException = typename TestFixture::TypeMismatchException;

  EXPECT_THROW(this->doc_["key1"].template As<bool>(), TypeMismatchException);
  EXPECT_NO_THROW(this->doc_["key1"].template As<double>());

  EXPECT_THROW(this->doc_["key2"].template As<bool>(), TypeMismatchException);
  EXPECT_THROW(this->doc_["key2"].template As<double>(), TypeMismatchException);
  EXPECT_THROW(this->doc_["key2"].template As<uint64_t>(),
               TypeMismatchException);

  EXPECT_THROW(this->doc_["key5"].template As<bool>(), TypeMismatchException);
  EXPECT_THROW(this->doc_["key5"].template As<uint64_t>(),
               TypeMismatchException);
  EXPECT_THROW(this->doc_["key5"].template As<int>(), TypeMismatchException);

  EXPECT_THROW(this->doc_["key6"].template As<double>(), TypeMismatchException);
  EXPECT_THROW(this->doc_["key6"].template As<uint64_t>(),
               TypeMismatchException);
  EXPECT_THROW(this->doc_["key6"].template As<int>(), TypeMismatchException);
}

TYPED_TEST_P(MemberAccess, MemberPaths) {
  using Value = typename TestFixture::Value;

  // copy/move constructor behaves as if it copied references not values
  Value js_copy = this->doc_;
  // check pointer equality of native objects
  EXPECT_TRUE(js_copy.DebugIsReferencingSameMemory(this->doc_));

  EXPECT_EQ(this->doc_.GetPath(), "/");
  EXPECT_EQ(this->doc_["key1"].GetPath(), "key1");
  EXPECT_EQ(this->doc_["key3"]["sub"].GetPath(), "key3.sub");
  EXPECT_EQ(this->doc_["key4"][2].GetPath(), "key4.[2]");

  EXPECT_EQ(js_copy.GetPath(), "/");
  EXPECT_EQ(js_copy["key1"].GetPath(), "key1");
  EXPECT_EQ(js_copy["key3"]["sub"].GetPath(), "key3.sub");
  EXPECT_EQ(js_copy["key4"][2].GetPath(), "key4.[2]");
}

TYPED_TEST_P(MemberAccess, MemberPathsByIterator) {
  auto it = this->doc_.begin();
  it++;
  it++;
  EXPECT_EQ(it->GetPath(), "key3");
  EXPECT_EQ(it->begin()->GetPath(), "key3.sub");
  it++;
  auto arr_it = it->begin();
  EXPECT_EQ((arr_it++)->GetPath(), "key4.[0]");
  EXPECT_EQ((++arr_it)->GetPath(), "key4.[2]");
}

TYPED_TEST_P(MemberAccess, MemberCount) { EXPECT_EQ(this->doc_.GetSize(), 6); }

TYPED_TEST_P(MemberAccess, HasMember) {
  EXPECT_TRUE(this->doc_.HasMember("key1"));
  EXPECT_FALSE(this->doc_.HasMember("keyX"));
  EXPECT_FALSE(this->doc_["keyX"].HasMember("key1"));
}

TYPED_TEST_P(MemberAccess, CopyMoveSubobject) {
  using Value = typename TestFixture::Value;

  // copy/move constructor behaves as if it copied references from subobjects
  Value v(this->doc_["key3"]);

  EXPECT_EQ(v, this->doc_["key3"]);
  EXPECT_TRUE(v.DebugIsReferencingSameMemory(this->doc_["key3"]));
}

TYPED_TEST_P(MemberAccess, IteratorOnNull) {
  using Value = typename TestFixture::Value;

  Value v;
  EXPECT_EQ(v.begin(), v.end());
}

TYPED_TEST_P(MemberAccess, IteratorOnMissingThrows) {
  using Value = typename TestFixture::Value;
  using MemberMissingException = typename TestFixture::MemberMissingException;

  Value v;
  EXPECT_THROW(v["missing_key"].begin(), MemberMissingException);
}

TYPED_TEST_P(MemberAccess, CloneValues) {
  using Value = typename TestFixture::Value;
  using ValueBuilder = typename TestFixture::ValueBuilder;

  Value v = this->doc_.Clone();
  EXPECT_EQ(v, this->doc_);

  this->doc_ = ValueBuilder(-1).ExtractValue();

  EXPECT_FALSE(v.DebugIsReferencingSameMemory(this->doc_));
}

TYPED_TEST_P(MemberAccess, CreateEmptyAndAccess) {
  using Value = typename TestFixture::Value;
  using ValueBuilder = typename TestFixture::ValueBuilder;
  using TypeMismatchException = typename TestFixture::TypeMismatchException;

  Value v;
  EXPECT_TRUE(v.IsRoot());
  EXPECT_EQ(v.GetPath(), "/");
  EXPECT_TRUE(v.IsNull());
  EXPECT_FALSE(v.HasMember("key_missing"));
  EXPECT_THROW(v.template As<bool>(), TypeMismatchException);
}

TYPED_TEST_P(MemberAccess, Subfield) {
  using Value = typename TestFixture::Value;
  using ValueBuilder = typename TestFixture::ValueBuilder;
  using TypeMismatchException = typename TestFixture::TypeMismatchException;
  using Type = typename TestFixture::Type;

  ValueBuilder body_builder(Type::kObject);

  Value old = this->doc_["key1"].Clone();
  EXPECT_EQ(old, this->doc_["key1"]);

  body_builder["key1"] = this->doc_["key1"];

  EXPECT_EQ(old, this->doc_["key1"]);
}

REGISTER_TYPED_TEST_CASE_P(
    MemberAccess,

    ChildBySquareBrakets, ChildBySquareBraketsTwice,
    ChildBySquareBraketsMissing, ChildBySquareBraketsMissingTwice,
    ChildBySquareBraketsArray, ChildBySquareBraketsBounds,

    IterateMemberNames, IterateAndCheckValues, IterateMembersAndCheckKey3,
    IterateMembersAndCheckKey4, IterateMembersAndCheckKey4Index,

    CheckPrimitiveTypes, CheckPrimitiveTypeExceptions, MemberPaths,
    MemberPathsByIterator, MemberCount, HasMember, CopyMoveSubobject,
    IteratorOnNull,

    IteratorOnMissingThrows, CloneValues, CreateEmptyAndAccess, Subfield);
