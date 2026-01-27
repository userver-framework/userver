#include <gtest/gtest-typed-test.h>

#include <algorithm>
#include <unordered_set>

USERVER_NAMESPACE_BEGIN

template <class T>
struct MemberAccess : public ::testing::Test {};
TYPED_TEST_SUITE_P(MemberAccess);

TYPED_TEST_P(MemberAccess, ChildBySquareBrackets) {
    using ValueBuilder = typename TestFixture::ValueBuilder;

    EXPECT_FALSE(this->doc["key1"].IsMissing());
    EXPECT_EQ(this->doc["key1"], ValueBuilder(1).ExtractValue());
}

TYPED_TEST_P(MemberAccess, ChildBySquareBracketsTwice) {
    using ValueBuilder = typename TestFixture::ValueBuilder;

    EXPECT_FALSE(this->doc["key3"]["sub"].IsMissing());
    EXPECT_EQ(this->doc["key3"]["sub"], ValueBuilder(-1).ExtractValue());
}

TYPED_TEST_P(MemberAccess, ChildBySquareBracketsMissing) {
    using MemberMissingException = typename TestFixture::MemberMissingException;

    EXPECT_NO_THROW(this->doc["key_missing"]);
    EXPECT_EQ(this->doc["key_missing"].GetPath(), "key_missing");
    EXPECT_TRUE(this->doc["key_missing"].IsMissing());
    EXPECT_FALSE(this->doc["key_missing"].IsNull());
    // possible false positive because of conditional in catch?
    // NOLINTNEXTLINE(misc-throw-by-value-catch-by-reference)
    EXPECT_THROW(this->doc["key_missing"].template As<bool>(), MemberMissingException);
}

TYPED_TEST_P(MemberAccess, ChildBySquareBracketsMissingTwice) {
    using MemberMissingException = typename TestFixture::MemberMissingException;

    EXPECT_NO_THROW(this->doc["key_missing"]["sub"]);
    EXPECT_EQ(this->doc["key_missing"]["sub"].GetPath(), "key_missing.sub");
    EXPECT_TRUE(this->doc["key_missing"]["sub"].IsMissing());
    EXPECT_FALSE(this->doc["key_missing"]["sub"].IsNull());
    // possible false positive because of conditional in catch?
    // NOLINTNEXTLINE(misc-throw-by-value-catch-by-reference)
    EXPECT_THROW(this->doc["key_missing"]["sub"].template As<bool>(), MemberMissingException);
}

TYPED_TEST_P(MemberAccess, ChildBySquareBracketsArray) {
    using ValueBuilder = typename TestFixture::ValueBuilder;

    EXPECT_EQ(this->doc["key4"][1], ValueBuilder(2).ExtractValue());
}

TYPED_TEST_P(MemberAccess, ChildBySquareBracketsBounds) {
    using OutOfBoundsException = typename TestFixture::OutOfBoundsException;

    // possible false positive because of conditional in catch?
    // NOLINTNEXTLINE(misc-throw-by-value-catch-by-reference)
    EXPECT_THROW(this->doc["key4"][9], OutOfBoundsException);
}

TYPED_TEST_P(MemberAccess, IterateMemberNames) {
    using TypeMismatchException = typename TestFixture::TypeMismatchException;

    EXPECT_TRUE(this->doc.IsObject());
    size_t ind = 1;
    std::unordered_set<std::string> all_keys;
    for (auto it = this->doc.begin(); it != this->doc.end(); ++it, ++ind) {
        const auto key = it.GetName();
        EXPECT_EQ(this->doc[key], *it) << "Failed for key " << key;
        EXPECT_TRUE(all_keys.insert(key).second) << "Already met key " << key;
        // possible false positive because of conditional in catch?
        // NOLINTNEXTLINE(misc-throw-by-value-catch-by-reference)
        EXPECT_THROW(it.GetIndex(), TypeMismatchException) << "Failed for key " << key;
    }
}

TYPED_TEST_P(MemberAccess, Items) {
    using formats::common::Items;
    size_t ind = 1;
    std::unordered_set<std::string> all_keys;
    for (const auto& [key, value] : Items(this->doc)) {
        EXPECT_EQ(this->doc[key], value) << "Failed for key " << key;
        EXPECT_TRUE(all_keys.insert(key).second) << "Already met key " << key;
        ++ind;
    }

    EXPECT_EQ(ind, 7);
}

TYPED_TEST_P(MemberAccess, IterateAndCheckValues) {
    using ValueBuilder = typename TestFixture::ValueBuilder;

    for (auto it = this->doc.begin(); it != this->doc.end(); ++it) {
        if (it.GetName() == "key1") {
            EXPECT_EQ(*it, ValueBuilder(1).ExtractValue());
        } else if (it.GetName() == "key2") {
            EXPECT_EQ(*it, ValueBuilder("val").ExtractValue());
        } else if (it.GetName() == "key3") {
            EXPECT_TRUE(it->IsObject());
            EXPECT_EQ((*it)["sub"], ValueBuilder(-1).ExtractValue());
        } else if (it.GetName() == "key4") {
            EXPECT_TRUE(it->IsArray());
        }
    }
}

TYPED_TEST_P(MemberAccess, IterateMembersAndCheckKey4) {
    using OutOfBoundsException = typename TestFixture::OutOfBoundsException;

    for (auto it = this->doc.begin(); it != this->doc.end(); ++it) {
        if (it.GetName() == "key4") {
            // possible false positive because of conditional in catch?
            // NOLINTNEXTLINE(misc-throw-by-value-catch-by-reference)
            EXPECT_THROW((*it)[9], OutOfBoundsException);
        }
    }
}

TYPED_TEST_P(MemberAccess, IterateMembersAndCheckKey4Index) {
    using TypeMismatchException = typename TestFixture::TypeMismatchException;

    for (auto it = this->doc.begin(); it != this->doc.end(); ++it) {
        if (it.GetName() == "key4") {
            EXPECT_TRUE(it->IsArray());

            size_t subind = 0;
            for (auto subit = it->begin(); subit != it->end(); ++subit, ++subind) {
                EXPECT_EQ(subit.GetIndex(), subind);
                // possible false positive because of conditional in catch?
                // NOLINTNEXTLINE(misc-throw-by-value-catch-by-reference)
                EXPECT_THROW(subit.GetName(), TypeMismatchException);
            }

            EXPECT_TRUE(it->IsArray()) << "Array iteration damaged the iterator";
            EXPECT_FALSE(it->IsEmpty()) << "Array iteration damaged the iterator";
            EXPECT_EQ(it->GetSize(), 3) << "Array iteration damaged the iterator";
        } else {
            EXPECT_FALSE(it->IsArray());
        }
    }
}

TYPED_TEST_P(MemberAccess, IterateMembersAndCheckKey4IndexPostincrement) {
    using TypeMismatchException = typename TestFixture::TypeMismatchException;

    for (auto it = this->doc.begin(); it != this->doc.end(); it++) {
        if (it.GetName() == "key4") {
            EXPECT_TRUE(it->IsArray());

            size_t subind = 0;
            for (auto subit = it->begin(); subit != it->end(); subit++, ++subind) {
                EXPECT_EQ(subit.GetIndex(), subind);
                // possible false positive because of conditional in catch?
                // NOLINTNEXTLINE(misc-throw-by-value-catch-by-reference)
                EXPECT_THROW(subit.GetName(), TypeMismatchException);
            }

            EXPECT_TRUE(it->IsArray()) << "Array iteration damaged the iterator";
            EXPECT_FALSE(it->IsEmpty()) << "Array iteration damaged the iterator";
            EXPECT_EQ(it->GetSize(), 3) << "Array iteration damaged the iterator";
        } else {
            EXPECT_FALSE(it->IsArray());
        }
    }
}

TYPED_TEST_P(MemberAccess, Algorithms) {
    auto it = std::find_if(this->doc.begin(), this->doc.end(), [](const auto& v) {
        return v.IsString() && v.template As<std::string>() == "val";
    });
    ASSERT_NE(it, this->doc.end()) << "failed to find array";
    EXPECT_EQ(it->template As<std::string>(), "val");
    EXPECT_EQ(it.GetName(), "key2");

    auto it_new = it;
    ++it_new;
    EXPECT_NE(it_new, it);
    if (this->doc.end() != it_new) {
        EXPECT_NE(it_new.GetName(), it.GetName());
    }

    EXPECT_EQ(it->template As<std::string>(), "val");

    ++it;
    EXPECT_EQ(it_new, it);
    if (this->doc.end() != it_new) {
        EXPECT_EQ(it_new.GetName(), it.GetName());
    }
}

TYPED_TEST_P(MemberAccess, CheckPrimitiveTypes) {
    EXPECT_TRUE(this->doc["key1"].IsUInt64());
    EXPECT_EQ(this->doc["key1"].template As<uint64_t>(), 1);

    EXPECT_TRUE(this->doc["key2"].IsString());
    EXPECT_EQ(this->doc["key2"].template As<std::string>(), "val");

    EXPECT_TRUE(this->doc["key3"].IsObject());
    EXPECT_TRUE(this->doc["key3"]["sub"].IsInt64());
    EXPECT_EQ(this->doc["key3"]["sub"].template As<int>(), -1);

    EXPECT_TRUE(this->doc["key4"].IsArray());
    EXPECT_TRUE(this->doc["key4"][0].IsUInt64());
    EXPECT_EQ(this->doc["key4"][0].template As<uint64_t>(), 1);

    EXPECT_TRUE(this->doc["key5"].IsDouble());
    EXPECT_DOUBLE_EQ(this->doc["key5"].template As<double>(), 10.5);
}

TYPED_TEST_P(MemberAccess, CheckPrimitiveTypeExceptions) {
    using TypeMismatchException = typename TestFixture::TypeMismatchException;

    // possible false positive because of conditional in catch?
    // NOLINTNEXTLINE(misc-throw-by-value-catch-by-reference)
    EXPECT_THROW(this->doc["key1"].template As<bool>(), TypeMismatchException);
    EXPECT_NO_THROW(this->doc["key1"].template As<double>());

    // NOLINTNEXTLINE(misc-throw-by-value-catch-by-reference)
    EXPECT_THROW(this->doc["key2"].template As<bool>(), TypeMismatchException);
    // NOLINTNEXTLINE(misc-throw-by-value-catch-by-reference)
    EXPECT_THROW(this->doc["key2"].template As<double>(), TypeMismatchException);
    // NOLINTNEXTLINE(misc-throw-by-value-catch-by-reference)
    EXPECT_THROW(this->doc["key2"].template As<uint64_t>(), TypeMismatchException);

    // NOLINTNEXTLINE(misc-throw-by-value-catch-by-reference)
    EXPECT_THROW(this->doc["key5"].template As<bool>(), TypeMismatchException);
    // NOLINTNEXTLINE(misc-throw-by-value-catch-by-reference)
    EXPECT_THROW(this->doc["key5"].template As<uint64_t>(), TypeMismatchException);
    // NOLINTNEXTLINE(misc-throw-by-value-catch-by-reference)
    EXPECT_THROW(this->doc["key5"].template As<int>(), TypeMismatchException);

    // NOLINTNEXTLINE(misc-throw-by-value-catch-by-reference)
    EXPECT_THROW(this->doc["key6"].template As<double>(), TypeMismatchException);
    // NOLINTNEXTLINE(misc-throw-by-value-catch-by-reference)
    EXPECT_THROW(this->doc["key6"].template As<uint64_t>(), TypeMismatchException);
    // NOLINTNEXTLINE(misc-throw-by-value-catch-by-reference)
    EXPECT_THROW(this->doc["key6"].template As<int>(), TypeMismatchException);
}

TYPED_TEST_P(MemberAccess, MemberPaths) {
    using Value = typename TestFixture::Value;

    // copy/move constructor behaves as if it copied references not values
    const Value js_copy = this->doc;
    // check pointer equality of native objects
    EXPECT_TRUE(js_copy.DebugIsReferencingSameMemory(this->doc));

    EXPECT_EQ(this->doc.GetPath(), "/");
    EXPECT_EQ(this->doc["key1"].GetPath(), "key1");
    EXPECT_EQ(this->doc["key3"]["sub"].GetPath(), "key3.sub");
    EXPECT_EQ(this->doc["key4"][2].GetPath(), "key4[2]");

    EXPECT_EQ(js_copy.GetPath(), "/");
    EXPECT_EQ(js_copy["key1"].GetPath(), "key1");
    EXPECT_EQ(js_copy["key3"]["sub"].GetPath(), "key3.sub");
    EXPECT_EQ(js_copy["key4"][2].GetPath(), "key4[2]");
}

TYPED_TEST_P(MemberAccess, MemberPathsByIterator) {
    EXPECT_EQ(this->doc["key3"].begin()->GetPath(), "key3.sub");

    auto arr_it = this->doc["key4"].begin();
    EXPECT_EQ((arr_it++)->GetPath(), "key4[0]");
    EXPECT_EQ((++arr_it)->GetPath(), "key4[2]");
}

TYPED_TEST_P(MemberAccess, MemberEmpty) {
    EXPECT_FALSE(this->doc.IsEmpty()) << "Map is not empty when it should be";
    EXPECT_FALSE(this->doc["key4"].IsEmpty()) << "Array is not empty when it should be";
}

TYPED_TEST_P(MemberAccess, MemberCount) {
    EXPECT_EQ(this->doc.GetSize(), 6) << "Incorrect size of a map";
    EXPECT_EQ(this->doc["key4"].GetSize(), 3) << "Incorrect size of an array";
}

TYPED_TEST_P(MemberAccess, HasMember) {
    EXPECT_TRUE(this->doc.HasMember("key1"));
    EXPECT_FALSE(this->doc.HasMember("keyX"));
    EXPECT_FALSE(this->doc["keyX"].HasMember("key1"));
}

TYPED_TEST_P(MemberAccess, CopyMoveSubobject) {
    using Value = typename TestFixture::Value;

    // copy/move constructor behaves as if it copied references from subobjects
    const Value v(this->doc["key3"]);

    EXPECT_EQ(v, this->doc["key3"]);
    EXPECT_TRUE(v.DebugIsReferencingSameMemory(this->doc["key3"]));
}

TYPED_TEST_P(MemberAccess, IteratorOnNull) {
    using Value = typename TestFixture::Value;

    const Value v;
    EXPECT_EQ(v.begin(), v.end());
}

TYPED_TEST_P(MemberAccess, IteratorOnMissingThrows) {
    using Value = typename TestFixture::Value;
    using MemberMissingException = typename TestFixture::MemberMissingException;

    const Value v;
    // possible false positive because of conditional in catch?
    // NOLINTNEXTLINE(misc-throw-by-value-catch-by-reference)
    EXPECT_THROW(v["missing_key"].begin(), MemberMissingException);
}

TYPED_TEST_P(MemberAccess, CloneValues) {
    using Value = typename TestFixture::Value;
    using ValueBuilder = typename TestFixture::ValueBuilder;

    const Value v = this->doc.Clone();
    EXPECT_EQ(v, this->doc);

    this->doc = ValueBuilder(-1).ExtractValue();

    EXPECT_FALSE(v.DebugIsReferencingSameMemory(this->doc));
}

TYPED_TEST_P(MemberAccess, CreateEmptyAndAccess) {
    using Value = typename TestFixture::Value;
    using TypeMismatchException = typename TestFixture::TypeMismatchException;

    const Value v;
    EXPECT_TRUE(v.IsRoot());
    EXPECT_EQ(v.GetPath(), "/");
    EXPECT_TRUE(v.IsNull());
    EXPECT_FALSE(v.HasMember("key_missing"));
    // possible false positive because of conditional in catch?
    // NOLINTNEXTLINE(misc-throw-by-value-catch-by-reference)
    EXPECT_THROW(v.template As<bool>(), TypeMismatchException);
}

TYPED_TEST_P(MemberAccess, Subfield) {
    using Value = typename TestFixture::Value;
    using ValueBuilder = typename TestFixture::ValueBuilder;
    using Type = typename TestFixture::Type;

    ValueBuilder body_builder(Type::kObject);

    const Value old = this->doc["key1"].Clone();
    EXPECT_EQ(old, this->doc["key1"]);

    body_builder["key1"] = this->doc["key1"];

    EXPECT_EQ(old, this->doc["key1"]);
}

TYPED_TEST_P(MemberAccess, ValueAssignment) {
    using Value = typename TestFixture::Value;

    Value v;
    v = this->doc["key4"];
    EXPECT_TRUE(v.IsArray());

    v = this->doc["key1"];
    EXPECT_FALSE(v.IsArray());
    EXPECT_TRUE(this->doc["key4"].IsArray());

    Value v2;
    v2 = v;
    v = this->doc["key4"];
    EXPECT_TRUE(v.IsArray());
    EXPECT_FALSE(v2.IsArray());

    Value v3;
    v = this->doc["key1"];
    v3 = std::move(v);
    v = this->doc["key4"];
    EXPECT_TRUE(v.IsArray());
    EXPECT_FALSE(v3.IsArray());
}

TYPED_TEST_P(MemberAccess, ConstFunctionsOnMissing) {
    using Value = typename TestFixture::Value;
    using MemberMissingException = typename TestFixture::MemberMissingException;

    const Value v = Value()["missing"];
    ASSERT_NO_THROW(v.IsMissing());
    ASSERT_TRUE(v.IsMissing());

    EXPECT_FALSE(v.IsNull());
    EXPECT_FALSE(v.IsBool());
    EXPECT_FALSE(v.IsInt());
    EXPECT_FALSE(v.IsInt64());
    EXPECT_FALSE(v.IsUInt64());
    EXPECT_FALSE(v.IsDouble());
    EXPECT_FALSE(v.IsString());
    EXPECT_FALSE(v.IsArray());
    EXPECT_FALSE(v.IsObject());

    // possible false positive because of conditional in catch?
    // NOLINTNEXTLINE(misc-throw-by-value-catch-by-reference)
    EXPECT_THROW((void)(v == v), MemberMissingException);
    // NOLINTNEXTLINE(misc-throw-by-value-catch-by-reference)
    EXPECT_THROW((void)(v != v), MemberMissingException);

    EXPECT_EQ(v.GetPath(), "missing");

    EXPECT_NO_THROW(v.IsRoot());
    EXPECT_FALSE(v.IsRoot());
    EXPECT_NO_THROW(v.HasMember("key_missing"));
}

TYPED_TEST_P(MemberAccess, AsWithDefault) {
    using Value = typename TestFixture::Value;

    EXPECT_EQ(Value()["missing"].template As<int>(42), 42);
    EXPECT_EQ(this->doc["key4"][1].template As<int>(42), 2);
}

TYPED_TEST_P(MemberAccess, RootAndPathOfCloned) {
    EXPECT_TRUE(this->doc.Clone().IsRoot());
    EXPECT_TRUE(this->doc.IsRoot());

    EXPECT_TRUE(this->doc["key4"].Clone().IsRoot());
    EXPECT_FALSE(this->doc["key4"].IsRoot());

    EXPECT_EQ(this->doc.Clone().GetPath(), this->doc.GetPath());
    EXPECT_EQ(this->doc.Clone().GetPath(), "/");

    EXPECT_EQ(this->doc["key4"].Clone().GetPath(), "/");
    EXPECT_EQ(this->doc["key4"].GetPath(), "key4");
}

REGISTER_TYPED_TEST_SUITE_P(
    MemberAccess,

    ChildBySquareBrackets,
    ChildBySquareBracketsTwice,
    ChildBySquareBracketsMissing,
    ChildBySquareBracketsMissingTwice,
    ChildBySquareBracketsArray,
    ChildBySquareBracketsBounds,

    Items,
    IterateMemberNames,
    IterateAndCheckValues,
    IterateMembersAndCheckKey4,
    IterateMembersAndCheckKey4Index,
    IterateMembersAndCheckKey4IndexPostincrement,
    Algorithms,

    CheckPrimitiveTypes,
    CheckPrimitiveTypeExceptions,
    MemberPaths,
    MemberPathsByIterator,
    MemberEmpty,
    MemberCount,
    HasMember,
    CopyMoveSubobject,
    IteratorOnNull,

    IteratorOnMissingThrows,
    CloneValues,
    CreateEmptyAndAccess,
    Subfield,
    ValueAssignment,
    ConstFunctionsOnMissing,
    AsWithDefault,
    RootAndPathOfCloned
);

USERVER_NAMESPACE_END
