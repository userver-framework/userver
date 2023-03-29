#include <gtest/gtest-typed-test.h>

#include <limits>

USERVER_NAMESPACE_BEGIN

template <class T>
struct MemberModify : public ::testing::Test {};
TYPED_TEST_SUITE_P(MemberModify);

TYPED_TEST_P(MemberModify, BuildNewValueEveryTime) {
  EXPECT_FALSE(this->GetBuiltValue().DebugIsReferencingSameMemory(
      this->GetBuiltValue()));
}

TYPED_TEST_P(MemberModify, CheckPrimitiveTypesChange) {
  this->builder_["key1"] = -100;
  EXPECT_EQ(this->GetBuiltValue()["key1"].template As<int>(), -100);

  this->builder_["key1"] = "100";
  EXPECT_EQ(this->GetBuiltValue()["key1"].template As<std::string>(), "100");
  this->builder_["key1"] = true;
  EXPECT_TRUE(this->GetBuiltValue()["key1"].template As<bool>());
}

TYPED_TEST_P(MemberModify, CheckNestedTypesChange) {
  this->builder_["key3"]["sub"] = false;
  EXPECT_FALSE(this->GetBuiltValue()["key3"]["sub"].template As<bool>());
  this->builder_["key3"] = -100;
  EXPECT_EQ(this->GetBuiltValue()["key3"].template As<int>(), -100);
  this->builder_["key3"] = this->FromString("{\"sub\":-1}");
  EXPECT_EQ(this->GetBuiltValue()["key3"]["sub"].template As<int>(), -1);
}

TYPED_TEST_P(MemberModify, CheckNestedArrayChange) {
  this->builder_["key4"][1] = 10;
  this->builder_["key4"][2] = 100;
  EXPECT_EQ(this->GetBuiltValue()["key4"][0].template As<int>(), 1);
  EXPECT_EQ(this->GetBuiltValue()["key4"][1].template As<int>(), 10);
  EXPECT_EQ(this->GetBuiltValue()["key4"][2].template As<int>(), 100);
}

TYPED_TEST_P(MemberModify, ArrayResize) {
  using OutOfBoundsException = typename TestFixture::OutOfBoundsException;

  this->builder_["key4"].Resize(4);
  EXPECT_FALSE(this->GetBuiltValue()["key4"].IsEmpty());
  EXPECT_EQ(this->GetBuiltValue()["key4"].GetSize(), 4);

  this->builder_["key4"][3] = 4;
  for (size_t i = 0; i < 4; ++i) {
    EXPECT_EQ(this->GetBuiltValue()["key4"][i].template As<int>(), i + 1);
  }

  this->builder_["key4"].Resize(1);

  EXPECT_FALSE(this->GetBuiltValue()["key4"].IsEmpty());
  EXPECT_EQ(this->GetBuiltValue()["key4"].GetSize(), 1);
  EXPECT_EQ(this->GetBuiltValue()["key4"][0].template As<int>(), 1);
  // possible false positive because of conditional in catch?
  // NOLINTNEXTLINE(misc-throw-by-value-catch-by-reference)
  EXPECT_THROW(this->GetBuiltValue()["key4"][2], OutOfBoundsException);
}

TYPED_TEST_P(MemberModify, ArrayFromNull) {
  using OutOfBoundsException = typename TestFixture::OutOfBoundsException;
  using ValueBuilder = typename TestFixture::ValueBuilder;

  this->builder_ = ValueBuilder();
  EXPECT_TRUE(this->GetBuiltValue().IsEmpty());
  EXPECT_EQ(this->GetBuiltValue().GetSize(), 0);

  this->builder_.Resize(1);
  EXPECT_FALSE(this->GetBuiltValue().IsEmpty());
  EXPECT_EQ(this->GetBuiltValue().GetSize(), 1);

  this->builder_[0] = 0;
  EXPECT_EQ(this->GetBuiltValue()[0].template As<int>(), 0);
  // possible false positive because of conditional in catch?
  // NOLINTNEXTLINE(misc-throw-by-value-catch-by-reference)
  EXPECT_THROW(this->GetBuiltValue()[2], OutOfBoundsException);
}

TYPED_TEST_P(MemberModify, ArrayPushBack) {
  using Type = typename TestFixture::Type;
  using ValueBuilder = typename TestFixture::ValueBuilder;

  this->builder_ = ValueBuilder(Type::kArray);

  const auto size = 5;
  for (auto i = 0; i < size; ++i) {
    this->builder_.PushBack(i);
  }
  EXPECT_EQ(this->GetBuiltValue().IsEmpty(), !size);
  EXPECT_EQ(this->GetBuiltValue().GetSize(), size);

  for (auto i = 0; i < size; ++i) {
    EXPECT_EQ(this->GetBuiltValue()[i].template As<int>(), i);
  }
}

TYPED_TEST_P(MemberModify, PushBackFromExisting) {
  this->builder_["key4"].PushBack(this->builder_["key1"]);
  EXPECT_FALSE(this->GetBuiltValue()["key4"].IsEmpty());
  EXPECT_EQ(this->GetBuiltValue()["key4"].GetSize(), 4);
  EXPECT_EQ(this->GetBuiltValue()["key4"][3].template As<int>(), 1);
  EXPECT_EQ(this->GetBuiltValue()["key1"].template As<int>(), 1);
}

TYPED_TEST_P(MemberModify, PushBackFromBefore) {
  this->builder_["key4"].PushBack(this->builder_["key4"][0]);
  EXPECT_FALSE(this->GetBuiltValue()["key4"].IsEmpty());
  EXPECT_EQ(this->GetBuiltValue()["key4"].GetSize(), 4);
  EXPECT_EQ(this->GetBuiltValue()["key4"][0].template As<int>(), 1);
  EXPECT_EQ(this->GetBuiltValue()["key4"][3].template As<int>(), 1);
}

TYPED_TEST_P(MemberModify, PushBackFromIterator) {
  this->builder_["key4"].PushBack(
      typename TestFixture::ValueBuilder{*this->builder_["key4"].begin()});
  EXPECT_FALSE(this->GetBuiltValue()["key4"].IsEmpty());
  EXPECT_EQ(this->GetBuiltValue()["key4"].GetSize(), 4);
  EXPECT_EQ(this->GetBuiltValue()["key4"][0].template As<int>(), 1);
  EXPECT_EQ(this->GetBuiltValue()["key4"][3].template As<int>(), 1);
}

TYPED_TEST_P(MemberModify, CopyExistingMember) {
  const auto key1 = this->builder_["key1"];
  this->builder_["key1_copy"] = key1;
  EXPECT_TRUE(this->GetBuiltValue().HasMember("key1"));
  EXPECT_TRUE(this->GetBuiltValue().HasMember("key1_copy"));
  EXPECT_EQ(this->GetBuiltValue()["key1"].template As<int>(), 1);
  EXPECT_EQ(this->GetBuiltValue()["key1_copy"].template As<int>(), 1);
}

TYPED_TEST_P(MemberModify, CopyExistingMemberByIt) {
  auto it = this->builder_.begin();
  while (it != this->builder_.end() && it.GetName() != "key1") ++it;
  ASSERT_NE(this->builder_.end(), it);

  this->builder_["key1_copy"] = *it;
  EXPECT_TRUE(this->GetBuiltValue().HasMember("key1"));
  EXPECT_TRUE(this->GetBuiltValue().HasMember("key1_copy"));
  EXPECT_EQ(this->GetBuiltValue()["key1"].template As<int>(), 1);
  EXPECT_EQ(this->GetBuiltValue()["key1_copy"].template As<int>(), 1);
}

// TODO: Fails on formats::yaml TAXICOMMON-2873
//
// TYPED_TEST_P(MemberModify, ModifyRemovedNode) {
//   auto builder = this->builder_["key1"];  // NB: not a copy
//   EXPECT_NO_THROW(builder = 0);
//   this->builder_.Remove("key1");
//   EXPECT_FALSE(this->builder_.HasMember("key1"));
//   EXPECT_THROW(builder = 0, typename TestFixture::MemberMissingException);
// }

TYPED_TEST_P(MemberModify, ContainerTypeChangeToScalar) {
  auto builder = this->builder_["key4"];  // NB: not a copy
  EXPECT_NO_THROW(builder.PushBack(0));
  this->builder_["key4"] = 0;
  // possible false positive because of conditional in catch?
  // NOLINTNEXTLINE(misc-throw-by-value-catch-by-reference)
  EXPECT_THROW(builder.PushBack(0),
               typename TestFixture::TypeMismatchException);
}

TYPED_TEST_P(MemberModify, ContainerTypeChangeArrToObj) {
  auto builder = this->builder_["key4"];  // NB: not a copy
  EXPECT_NO_THROW(builder.PushBack(0));
  this->builder_["key4"] =
      typename TestFixture::ValueBuilder{formats::common::Type::kObject};
  // possible false positive because of conditional in catch?
  // NOLINTNEXTLINE(misc-throw-by-value-catch-by-reference)
  EXPECT_THROW(builder.PushBack(0),
               typename TestFixture::TypeMismatchException);
  EXPECT_NO_THROW(builder["test"] = 0);
}

TYPED_TEST_P(MemberModify, ContainerTypeChangeObjToArr) {
  auto builder = this->builder_["key3"];  // NB: not a copy
  EXPECT_NO_THROW(builder["test"] = 0);
  this->builder_["key3"] =
      typename TestFixture::ValueBuilder{formats::common::Type::kArray};
  // possible false positive because of conditional in catch?
  // NOLINTNEXTLINE(misc-throw-by-value-catch-by-reference)
  EXPECT_THROW(builder["test"] = 0,
               typename TestFixture::TypeMismatchException);
  EXPECT_NO_THROW(builder.PushBack(0));
}

TYPED_TEST_P(MemberModify, PushBackWrongTypeThrows) {
  using TypeMismatchException = typename TestFixture::TypeMismatchException;

  // possible false positive because of conditional in catch?
  // NOLINTNEXTLINE(misc-throw-by-value-catch-by-reference)
  EXPECT_THROW(this->builder_["key1"].PushBack(1), TypeMismatchException);
}

TYPED_TEST_P(MemberModify, ExtractFromSubBuilderThrows) {
  using Exception = typename TestFixture::Exception;

  auto bld = this->builder_["key1"];
  // possible false positive because of conditional in catch?
  // NOLINTNEXTLINE(misc-throw-by-value-catch-by-reference)
  EXPECT_THROW(bld.ExtractValue(), Exception);
}

TYPED_TEST_P(MemberModify, ObjectIteratorModify) {
  const std::size_t offset = 3;
  std::size_t sum = 0;
  {
    std::size_t size = 0;
    auto it = this->builder_.begin();
    for (; it != this->builder_.end(); ++it, ++size) {
      *it = offset + size;
      sum += offset + size;
    }
  }

  {
    auto values = this->GetBuiltValue();
    for (const auto& v : values) {
      sum -= v.template As<int>();
    }
    EXPECT_EQ(sum, 0);
  }
}

TYPED_TEST_P(MemberModify, MemberEmpty) {
  EXPECT_FALSE(this->builder_.IsEmpty());
}

TYPED_TEST_P(MemberModify, MemberCount) {
  EXPECT_EQ(this->builder_.GetSize(), 5);
}

TYPED_TEST_P(MemberModify, NonArrayThrowIsEmpty) {
  using ValueBuilder = typename TestFixture::ValueBuilder;
  using TypeMismatchException = typename TestFixture::TypeMismatchException;

  ValueBuilder bld(true);
  // possible false positive because of conditional in catch?
  // NOLINTNEXTLINE(misc-throw-by-value-catch-by-reference)
  EXPECT_THROW(bld.IsEmpty(), TypeMismatchException);
}

TYPED_TEST_P(MemberModify, NonArrayThrowGetSize) {
  using ValueBuilder = typename TestFixture::ValueBuilder;
  using TypeMismatchException = typename TestFixture::TypeMismatchException;

  ValueBuilder bld(true);
  // possible false positive because of conditional in catch?
  // NOLINTNEXTLINE(misc-throw-by-value-catch-by-reference)
  EXPECT_THROW(bld.GetSize(), TypeMismatchException);
}

TYPED_TEST_P(MemberModify, ArrayIteratorRead) {
  using ValueBuilder = typename TestFixture::ValueBuilder;

  this->builder_ = ValueBuilder();
  const auto size = 10;
  this->builder_.Resize(size);

  for (auto i = 0; i < size; ++i) {
    this->builder_[i] = i;
  }

  auto it = this->GetBuiltValue().begin();
  for (auto i = 0; i < size; ++i, ++it) {
    EXPECT_EQ(it->template As<int>(), i) << "Failed at index " << i;
  }
  EXPECT_EQ(this->builder_.IsEmpty(), !size);
  EXPECT_EQ(this->builder_.GetSize(), size);
}

TYPED_TEST_P(MemberModify, ArrayIteratorModify) {
  using ValueBuilder = typename TestFixture::ValueBuilder;

  this->builder_ = ValueBuilder();
  const auto size = 10;
  this->builder_.Resize(size);

  const size_t offset = 3;
  {
    // Cannot initialize values of array
    // with iterators (need to first set values with operator[])
    for (auto i = 0; i < size; ++i) {
      this->builder_[i] = 0;
    }

    auto it = this->builder_.begin();
    for (auto i = offset; it != this->builder_.end(); ++it, ++i) {
      *it = i;
    }
  }

  {
    auto it = this->GetBuiltValue().begin();
    for (auto i = 0; i < size; ++i, ++it) {
      EXPECT_EQ(it->template As<int>(), i + offset) << "Failed at index " << i;
    }
  }
  EXPECT_EQ(this->builder_.IsEmpty(), !size);
  EXPECT_EQ(this->builder_.GetSize(), size);
}

TYPED_TEST_P(MemberModify, CreateSpecificType) {
  using ValueBuilder = typename TestFixture::ValueBuilder;
  using TypeMismatchException = typename TestFixture::TypeMismatchException;
  using Type = typename TestFixture::Type;

  ValueBuilder js_obj(Type::kObject);
  // possible false positive because of conditional in catch?
  // NOLINTNEXTLINE(misc-throw-by-value-catch-by-reference)
  EXPECT_THROW(this->GetValue(js_obj)[0], TypeMismatchException);

  ValueBuilder js_arr(Type::kArray);
  // NOLINTNEXTLINE(misc-throw-by-value-catch-by-reference)
  EXPECT_THROW(this->GetValue(js_arr)["key"], TypeMismatchException);
}

TYPED_TEST_P(MemberModify, IteratorOutlivesRoot) {
  using Value = typename TestFixture::Value;

  auto it = this->GetBuiltValue().begin();
  {
    Value v = this->GetBuiltValue();
    it = v["key4"].begin();
    EXPECT_EQ(it->GetPath(), "key4[0]");
  }
  EXPECT_EQ((++it)->GetPath(), "key4[1]");
}

TYPED_TEST_P(MemberModify, SubdocOutlivesRoot) {
  using Value = typename TestFixture::Value;

  Value v = this->GetBuiltValue()["key3"];
  EXPECT_TRUE(v.HasMember("sub"));
}

TYPED_TEST_P(MemberModify, MoveValueBuilder) {
  using ValueBuilder = typename TestFixture::ValueBuilder;

  ValueBuilder v = std::move(this->builder_);
  EXPECT_FALSE(this->GetValue(v).IsNull());
  EXPECT_TRUE(this->GetBuiltValue().IsNull());
}

TYPED_TEST_P(MemberModify, CheckSubobjectChange) {
  using ValueBuilder = typename TestFixture::ValueBuilder;

  ValueBuilder v = this->GetBuiltValue();
  this->builder_["key4"] = v["key3"];
  EXPECT_TRUE(this->GetBuiltValue()["key4"].HasMember("sub"));
  EXPECT_EQ(this->GetBuiltValue()["key4"]["sub"].template As<int>(), -1);
  EXPECT_TRUE(this->GetValue(v)["key3"].HasMember("sub"));
  EXPECT_EQ(this->GetValue(v)["key3"]["sub"].template As<int>(), -1);
}

TYPED_TEST_P(MemberModify, TypeCheckMinMax) {
  using ValueBuilder = typename TestFixture::ValueBuilder;

  ValueBuilder bld;
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

  bld["float"] = 0.0F;
  bld["double"] = 0.0;

  auto v = this->GetValue(bld);
  EXPECT_EQ(v["int8_t"].template As<int>(), std::numeric_limits<int8_t>::min());
  EXPECT_EQ(v["uint8_t"].template As<uint64_t>(),
            std::numeric_limits<uint8_t>::max());
  EXPECT_EQ(v["int16_t"].template As<int>(),
            std::numeric_limits<int16_t>::min());
  EXPECT_EQ(v["uint16_t"].template As<uint64_t>(),
            std::numeric_limits<uint16_t>::max());
  EXPECT_EQ(v["int32_t"].template As<int>(),
            std::numeric_limits<int32_t>::min());
  EXPECT_EQ(v["uint32_t"].template As<uint64_t>(),
            std::numeric_limits<uint32_t>::max());
  EXPECT_EQ(v["int64_t"].template As<int64_t>(),
            std::numeric_limits<int64_t>::min());
  EXPECT_EQ(v["uint64_t"].template As<uint64_t>(),
            std::numeric_limits<uint64_t>::max());
  EXPECT_EQ(v["size_t"].template As<size_t>(),
            std::numeric_limits<size_t>::max());
  EXPECT_EQ(v["long"].template As<int64_t>(), std::numeric_limits<long>::min());
  EXPECT_EQ(v["long long"].template As<int64_t>(),
            std::numeric_limits<long long>::min());

  EXPECT_EQ(v["float"].template As<float>(), 0.0);
  EXPECT_EQ(v["double"].template As<double>(), 0.0);
}

TYPED_TEST_P(MemberModify, CannotBuildFromMissing) {
  using ValueBuilder = typename TestFixture::ValueBuilder;
  using Value = typename TestFixture::Value;
  using MemberMissingException = typename TestFixture::MemberMissingException;

  Value v;
  ValueBuilder bld;
  // possible false positive because of conditional in catch?
  // NOLINTNEXTLINE(misc-throw-by-value-catch-by-reference)
  EXPECT_THROW(bld = v["missing_key"], MemberMissingException);
}

TYPED_TEST_P(MemberModify, IsCheckObjectValidFunction) {
  using ValueBuilder = typename TestFixture::ValueBuilder;
  using TypeMismatchException = typename TestFixture::TypeMismatchException;
  // This test is actually for absence in CheckObject internal errors,
  // and not that it returns true for objects and false for not-objects.
  {
    ValueBuilder builder{formats::common::Type::kObject};
    EXPECT_NO_THROW(builder.ExtractValue().CheckObject());
  }
  {
    // Check that scalar is not object
    ValueBuilder builder{formats::common::Type::kObject};
    builder["a"] = 3;
    // possible false positive because of conditional in catch?
    // NOLINTNEXTLINE(misc-throw-by-value-catch-by-reference)
    EXPECT_THROW(builder.ExtractValue()["a"].CheckObject(),
                 TypeMismatchException);
  }
  {
    ValueBuilder builder{formats::common::Type::kNull};
    // NOLINTNEXTLINE(misc-throw-by-value-catch-by-reference)
    EXPECT_THROW(builder.ExtractValue().CheckObject(), TypeMismatchException);
  }
}

REGISTER_TYPED_TEST_SUITE_P(
    MemberModify,

    BuildNewValueEveryTime, CheckPrimitiveTypesChange, CheckNestedTypesChange,

    CheckNestedArrayChange, ArrayResize, ArrayFromNull, ArrayPushBack,

    PushBackFromExisting, PushBackFromBefore, PushBackFromIterator,
    CopyExistingMember, CopyExistingMemberByIt, /* ModifyRemovedNode, */
    ContainerTypeChangeToScalar, ContainerTypeChangeArrToObj,
    ContainerTypeChangeObjToArr,

    PushBackWrongTypeThrows, ExtractFromSubBuilderThrows, ObjectIteratorModify,
    MemberEmpty, MemberCount,

    NonArrayThrowIsEmpty, NonArrayThrowGetSize, ArrayIteratorRead,
    ArrayIteratorModify, CreateSpecificType, IteratorOutlivesRoot,

    SubdocOutlivesRoot, MoveValueBuilder, CheckSubobjectChange, TypeCheckMinMax,
    CannotBuildFromMissing,

    IsCheckObjectValidFunction);

USERVER_NAMESPACE_END
