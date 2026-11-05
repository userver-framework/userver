#include <gtest/gtest.h>

#include <array>
#include <atomic>
#include <barrier>
#include <sstream>
#include <string>
#include <string_view>
#include <thread>
#include <utility>
#include <vector>

#include <userver/formats/json/exception.hpp>
#include <userver/formats/json/inline.hpp>
#include <userver/formats/json/serialize.hpp>
#include <userver/formats/json/value_builder.hpp>

#include <formats/common/member_access_test.hpp>

USERVER_NAMESPACE_BEGIN

namespace {

// clang-format off

/// [Sample json inline construction functions]
const auto kDoc = formats::json::MakeObject(
    "key1", 1,
    "key2", "val",
    "key3", formats::json::MakeObject("sub", -1),
    "key4", formats::json::MakeArray(1, 2, 3),
    "key5", 10.5,
    "key6", false
);
/// [Sample json inline construction functions]

// clang-format on

}  // namespace

template <>
struct MemberAccess<formats::json::Value> : public ::testing::Test {
    // XXX: not working from base class in clang-7 (decltype?)
    static_assert(
        !std::is_assignable_v<decltype(*std::declval<formats::json::Value>().begin()), formats::json::Value>,
        "Value iterators are not assignable"
    );

    MemberAccess()
        : doc(kDoc)
    {}
    formats::json::Value doc;

    using ValueBuilder = formats::json::ValueBuilder;
    using Value = formats::json::Value;
    using Type = formats::json::Type;

    using ParseException = formats::json::ParseException;
    using TypeMismatchException = formats::json::TypeMismatchException;
    using OutOfBoundsException = formats::json::OutOfBoundsException;
    using MemberMissingException = formats::json::MemberMissingException;
};

INSTANTIATE_TYPED_TEST_SUITE_P(FormatsJson, MemberAccess, formats::json::Value);

struct FormatsJsonSpecificMemberAccess : public MemberAccess<formats::json::Value> {};

TEST_F(FormatsJsonSpecificMemberAccess, CheckPrimitiveTypes) {
    using TypeMismatchException = formats::json::TypeMismatchException;

    // Differs from YAML:
    EXPECT_FALSE(doc["key3"]["sub"].IsUInt64());
    EXPECT_THROW(doc["key1"].As<std::string>(), TypeMismatchException);
    EXPECT_THROW(doc["key5"].As<std::string>(), TypeMismatchException);
    EXPECT_THROW(doc["key6"].As<std::string>(), TypeMismatchException);
}

TEST_F(FormatsJsonSpecificMemberAccess, Items) {
    for ([[maybe_unused]] const auto& [key, value] : Items(doc)) {
    }
}

TEST_F(FormatsJsonSpecificMemberAccess, IterateAndCheckValues) {
    const auto arr = doc["key4"];

    int i = 2;
    int val = 3;
    for (auto it = arr.rbegin(); it != arr.rend(); ++it, --i, --val) {
        EXPECT_EQ(it->As<int>(), val);
        EXPECT_EQ(it.GetIndex(), i);
    }
}

TEST_F(FormatsJsonSpecificMemberAccess, CheckPrimitiveTypeExceptions) {
    EXPECT_THROW(doc["key1"].rbegin(), TypeMismatchException);
    EXPECT_THROW(doc["key1"].rend(), TypeMismatchException);

    EXPECT_THROW(doc["key2"].rbegin(), TypeMismatchException);
    EXPECT_THROW(doc["key2"].rend(), TypeMismatchException);

    EXPECT_THROW(doc["key3"].rbegin(), TypeMismatchException);
    EXPECT_THROW(doc["key3"].rend(), TypeMismatchException);

    EXPECT_THROW(doc["key5"].rbegin(), TypeMismatchException);
    EXPECT_THROW(doc["key5"].rend(), TypeMismatchException);

    EXPECT_THROW(doc["key6"].rbegin(), TypeMismatchException);
    EXPECT_THROW(doc["key6"].rend(), TypeMismatchException);
}

TEST_F(FormatsJsonSpecificMemberAccess, TypeMismatchExceptionAccessToAttributes) {
    EXPECT_THROW(doc["key1"].As<std::string>(), TypeMismatchException);
    try {
        doc["key1"].As<std::string>();
    } catch (const TypeMismatchException& e) {
        EXPECT_EQ(e.GetActual(), "kIntValue");
        EXPECT_EQ(e.GetExpected(), "kStringValue");
        EXPECT_EQ(e.GetPath(), "key1");
    }
}

TEST_F(FormatsJsonSpecificMemberAccess, OutOfBoundsExceptionAccessToAttributes) {
    const auto arr = doc["key4"];
    EXPECT_THROW(arr[4], OutOfBoundsException);
    try {
        arr[4];
    } catch (const OutOfBoundsException& e) {
        EXPECT_EQ(e.GetPath(), "key4");
    }
}

TEST_F(FormatsJsonSpecificMemberAccess, ArrayIteratorRetainsCopiedValuesAfterAdvancing) {
    const auto array = doc["key4"];
    auto it = array.begin();
    const auto first = *it;
    EXPECT_EQ(it->As<int>(), 1);
    ++it;
    ++it;
    EXPECT_EQ(it->As<int>(), 3);
    EXPECT_EQ(it->GetPath(), "key4[2]");
    EXPECT_EQ(first.As<int>(), 1);
    EXPECT_EQ(first.GetPath(), "key4[0]");
    EXPECT_THROW(it->As<std::string>(), TypeMismatchException);
    ++it;
    EXPECT_EQ(it, array.end());
}

TEST_F(FormatsJsonSpecificMemberAccess, ArrayIteratorPostincrementRetainsPreviousValue) {
    const auto array = doc["key4"];
    auto it = array.begin();
    EXPECT_EQ(it->As<int>(), 1);
    const auto first = *it++;
    const auto second = *it++;
    EXPECT_EQ(first.As<int>(), 1);
    EXPECT_EQ(first.GetPath(), "key4[0]");
    EXPECT_EQ(second.As<int>(), 2);
    EXPECT_EQ(second.GetPath(), "key4[1]");
    EXPECT_EQ(it->As<int>(), 3);
}

TEST_F(FormatsJsonSpecificMemberAccess, ArrayIteratorCopiesAdvanceIndependently) {
    const auto array = doc["key4"];
    auto it = array.begin();
    EXPECT_EQ(it->As<int>(), 1);
    auto copy = it;
    ++it;
    EXPECT_EQ(it->As<int>(), 2);
    EXPECT_EQ(copy->As<int>(), 1);
    ++copy;
    ++copy;
    EXPECT_EQ(copy->As<int>(), 3);
    EXPECT_EQ(it->As<int>(), 2);
}

TEST_F(FormatsJsonSpecificMemberAccess, ArrayIteratorAssignmentReplacesCachedRoot) {
    auto it = doc["key4"].begin();
    EXPECT_EQ(it->As<int>(), 1);
    const auto replacement = formats::json::MakeObject("other", formats::json::MakeArray(4, 5));
    auto other = replacement["other"].begin();
    EXPECT_EQ(other->As<int>(), 4);
    it = other;
    ++it;
    EXPECT_EQ(it->As<int>(), 5);
    EXPECT_EQ(it->GetPath(), "other[1]");
    it = std::move(other);
    EXPECT_EQ(it->As<int>(), 4);
    EXPECT_EQ(it->GetPath(), "other[0]");
}

TEST_F(FormatsJsonSpecificMemberAccess, ReverseArrayIteratorRetainsValuesAndPaths) {
    const auto array = doc["key4"];
    auto it = array.rbegin();
    const auto last = *it;
    const auto previous = *it++;
    EXPECT_EQ(previous.As<int>(), 3);
    EXPECT_EQ(it->As<int>(), 2);
    EXPECT_EQ(it->GetPath(), "key4[1]");
    ++it;
    EXPECT_EQ(it->As<int>(), 1);
    EXPECT_EQ(it->GetPath(), "key4[0]");
    ++it;
    EXPECT_EQ(it, array.rend());
    EXPECT_EQ(last.As<int>(), 3);
    EXPECT_EQ(last.GetPath(), "key4[2]");
}

TEST_F(FormatsJsonSpecificMemberAccess, MoveArrayIteratorAfterCachedAdvance) {
    const auto array = doc["key4"];
    auto source = array.begin();
    const auto first = *source;
    ++source;
    auto moved = std::move(source);
    EXPECT_EQ(moved->As<int>(), 2);
    EXPECT_EQ(moved->GetPath(), "key4[1]");
    ++moved;
    EXPECT_EQ(moved->As<int>(), 3);
    EXPECT_EQ(first.As<int>(), 1);
    source = array.begin();
    EXPECT_EQ(source->As<int>(), 1);
}

TEST_F(FormatsJsonSpecificMemberAccess, MoveReverseIteratorAfterCachedAdvance) {
    const auto array = doc["key4"];
    auto source = array.rbegin();
    const auto last = *source;
    ++source;
    auto moved = std::move(source);
    EXPECT_EQ(moved->As<int>(), 2);
    EXPECT_EQ(moved->GetPath(), "key4[1]");
    ++moved;
    EXPECT_EQ(moved->As<int>(), 1);
    EXPECT_EQ(last.As<int>(), 3);
    source = array.rbegin();
    EXPECT_EQ(source->As<int>(), 3);
}

TEST_F(FormatsJsonSpecificMemberAccess, MoveObjectIteratorAfterCachedAdvance) {
    const auto object = formats::json::MakeObject("first", 1, "second", 2);
    auto source = object.begin();
    const auto first = *source;
    ++source;
    formats::json::Value::const_iterator moved;
    moved = std::move(source);
    EXPECT_EQ(moved.GetName(), "second");
    EXPECT_EQ(moved->As<int>(), 2);
    EXPECT_EQ(moved->GetPath(), "second");
    ++moved;
    EXPECT_EQ(moved, object.end());
    EXPECT_EQ(first.As<int>(), 1);
    source = object.begin();
    EXPECT_EQ(source->As<int>(), 1);
}

TEST_F(FormatsJsonSpecificMemberAccess, ObjectIteratorRetainsCopiedMemberAndUpdatesPath) {
    const auto object = formats::json::MakeObject("first", 1, "second", 2, "third", 3);
    auto it = object.begin();
    const auto first = *it;
    ++it;
    EXPECT_EQ(it.GetName(), "second");
    EXPECT_EQ(it->As<int>(), 2);
    EXPECT_EQ(it->GetPath(), "second");
    const auto second = *it++;
    EXPECT_EQ(it.GetName(), "third");
    EXPECT_EQ(it->As<int>(), 3);
    EXPECT_EQ(it->GetPath(), "third");
    EXPECT_EQ(first.As<int>(), 1);
    EXPECT_EQ(first.GetPath(), "first");
    EXPECT_EQ(second.As<int>(), 2);
    EXPECT_EQ(second.GetPath(), "second");
}

TEST_F(FormatsJsonSpecificMemberAccess, CopiedArrayElementsOutliveContainerAndIterators) {
    const auto values = [] {
        const auto object = formats::json::MakeObject("array", formats::json::MakeArray(1, 2, 3));
        std::vector<formats::json::Value> result;
        for (const auto& value : object["array"]) {
            result.push_back(value);
        }
        return result;
    }();
    ASSERT_EQ(values.size(), 3);
    EXPECT_EQ(values[0].As<int>(), 1);
    EXPECT_EQ(values[1].As<int>(), 2);
    EXPECT_EQ(values[2].As<int>(), 3);
    EXPECT_EQ(values[0].GetPath(), "array[0]");
    EXPECT_EQ(values[2].GetPath(), "array[2]");
}

TEST_F(FormatsJsonSpecificMemberAccess, ConcurrentIteratorsKeepCopiedValuesIndependent) {
    constexpr std::size_t kThreadCount = 4;
    constexpr std::size_t kIterations = 1000;
    const auto object = formats::json::MakeObject("array", formats::json::MakeArray(1, 2, 3));
    const auto array = object["array"];
    std::barrier start(kThreadCount);
    std::atomic<std::size_t> failures{0};
    std::array<std::thread, kThreadCount> readers;
    for (auto& reader : readers) {
        reader = std::thread([&] {
            start.arrive_and_wait();
            for (std::size_t iteration = 0; iteration < kIterations; ++iteration) {
                auto it = array.begin();
                const auto first = *it;
                auto copy = it;
                ++it;
                const auto second = *it++;
                auto reverse = array.rbegin();
                const auto last = *reverse;
                ++reverse;
                if (first.As<int>() != 1 || first.GetPath() != "array[0]" || copy->As<int>() != 1 ||
                    second.As<int>() != 2 || second.GetPath() != "array[1]" || it->As<int>() != 3 ||
                    it->GetPath() != "array[2]" || last.As<int>() != 3 || last.GetPath() != "array[2]" ||
                    reverse->As<int>() != 2 || reverse->GetPath() != "array[1]")
                {
                    failures.fetch_add(1, std::memory_order_relaxed);
                }
            }
        });
    }
    for (auto& reader : readers) {
        reader.join();
    }
    EXPECT_EQ(failures.load(std::memory_order_relaxed), 0);
}

namespace {

static_assert(std::is_same_v<formats::json::Value::IterTraits::ValueType, formats::json::Value>);
static_assert(std::is_same_v<formats::json::ValueBuilder::IterTraits::ValueType, formats::json::ValueBuilder>);

formats::json::Value MakeIteratorPathDocument() {
    return formats::json::FromString(R"json({
        "outer": {
            "array": [1, "text", null, {"leaf": 3}, [4], true],
            "object": {
                "number": 1, "string": "text", "nil": null,
                "object": {"leaf": 3}, "array": [4], "bool": true
            },
            "rows": [{"leaf": 1}, {"leaf": 2}, null, {"leaf": 4}],
            "members": {
                "first": {"leaf": 1}, "second": {"leaf": 2},
                "nil": null, "last": {"leaf": 4}
            }
        }
    })json");
}

void ExpectMissingPath(const formats::json::Value& value, std::string_view expected) {
    ASSERT_TRUE(value.IsMissing());
    EXPECT_FALSE(value.IsNull());
    EXPECT_EQ(value.GetPath(), expected);
    EXPECT_EQ(value.GetPath(), expected);
    try {
        (void)value.As<int>();
        ADD_FAILURE() << "Expected MemberMissingException for " << expected;
    } catch (const formats::json::MemberMissingException& exception) {
        EXPECT_EQ(exception.GetPath(), expected);
    }
}

}  // namespace

TEST(FormatsJsonIteratorPaths, ForwardArrayPathsForEveryValue) {
    const auto document = MakeIteratorPathDocument();
    const auto array = document["outer"]["array"];
    std::size_t index = 0;
    const auto end = array.end();
    for (auto it = array.begin(); it != end; ++it, ++index) {
        SCOPED_TRACE(index);
        const auto expected = "outer.array[" + std::to_string(index) + "]";
        const auto& value = *it;
        EXPECT_EQ(value.GetPath(), expected);
        EXPECT_EQ(it->GetPath(), expected);
        EXPECT_EQ(it.GetIndex(), index);
        EXPECT_FALSE(value.IsMissing());
        EXPECT_EQ(value.IsNull(), index == 2);
        EXPECT_TRUE(value.DebugIsReferencingSameMemory(array[index]));
    }
    EXPECT_EQ(index, 6);
}

TEST(FormatsJsonIteratorPaths, ForwardObjectPathsForEveryValue) {
    const auto document = MakeIteratorPathDocument();
    const auto object = document["outer"]["object"];
    constexpr std::array<std::string_view, 6> names{"number", "string", "nil", "object", "array", "bool"};
    std::size_t index = 0;
    const auto end = object.end();
    for (auto it = object.begin(); it != end; ++it, ++index) {
        ASSERT_LT(index, names.size());
        SCOPED_TRACE(names[index]);
        const auto expected = "outer.object." + std::string{names[index]};
        EXPECT_EQ(it.GetName(), names[index]);
        EXPECT_EQ((*it).GetPath(), expected);
        EXPECT_EQ(it->GetPath(), expected);
        EXPECT_EQ(it->IsNull(), names[index] == "nil");
        EXPECT_TRUE(it->DebugIsReferencingSameMemory(object[names[index]]));
    }
    EXPECT_EQ(index, names.size());
}

TEST(FormatsJsonIteratorPaths, ReverseArrayPathsForEveryValue) {
    const auto document = MakeIteratorPathDocument();
    const auto array = document["outer"]["array"];
    std::size_t remaining = array.GetSize();
    const auto end = array.rend();
    for (auto it = array.rbegin(); it != end; ++it) {
        ASSERT_GT(remaining, 0);
        const auto index = --remaining;
        const auto expected = "outer.array[" + std::to_string(index) + "]";
        SCOPED_TRACE(expected);
        EXPECT_EQ(it->GetPath(), expected);
        EXPECT_EQ((*it).GetPath(), expected);
        EXPECT_EQ(it.GetIndex(), index);
        EXPECT_TRUE(it->DebugIsReferencingSameMemory(array[index]));
    }
    EXPECT_EQ(remaining, 0);
}

TEST(FormatsJsonIteratorPaths, MissingArrayChildrenOutliveDocumentAndIterator) {
    const auto missing = [] {
        const auto document = MakeIteratorPathDocument();
        const auto array = document["outer"]["rows"];
        std::vector<formats::json::Value> result;
        for (const auto& value : array) {
            result.push_back(value["absent"]["child"]);
        }
        return result;
    }();
    ASSERT_EQ(missing.size(), 4);
    for (std::size_t index = 0; index < missing.size(); ++index) {
        SCOPED_TRACE(index);
        ExpectMissingPath(missing[index], "outer.rows[" + std::to_string(index) + "].absent.child");
    }
}

TEST(FormatsJsonIteratorPaths, MissingObjectChildrenOutliveDocumentAndIterator) {
    const auto missing = [] {
        const auto document = MakeIteratorPathDocument();
        const auto object = document["outer"]["members"];
        std::vector<formats::json::Value> result;
        const auto end = object.end();
        for (auto it = object.begin(); it != end; ++it) {
            result.push_back((*it)["absent"]["child"]);
        }
        return result;
    }();
    constexpr std::array<std::string_view, 4> names{"first", "second", "nil", "last"};
    ASSERT_EQ(missing.size(), names.size());
    for (std::size_t index = 0; index < missing.size(); ++index) {
        SCOPED_TRACE(names[index]);
        ExpectMissingPath(missing[index], "outer.members." + std::string{names[index]} + ".absent.child");
    }
}

TEST(FormatsJsonIteratorPaths, ReverseMissingChildrenKeepDetachedPaths) {
    const auto document = MakeIteratorPathDocument();
    const auto array = document["outer"]["rows"];
    std::vector<formats::json::Value> missing;
    const auto end = array.rend();
    for (auto it = array.rbegin(); it != end; ++it) {
        missing.push_back((*it)["absent"]["child"]);
    }
    ASSERT_EQ(missing.size(), 4);
    for (std::size_t index = 0; index < missing.size(); ++index) {
        ExpectMissingPath(missing[index], "outer.rows[" + std::to_string(3 - index) + "].absent.child");
    }
}

TEST(FormatsJsonIteratorPaths, CopiedPresentValuesKeepPathsAfterDocumentDestruction) {
    const auto values = [] {
        const auto document = MakeIteratorPathDocument();
        std::vector<formats::json::Value> result;
        for (const auto& value : document["outer"]["array"]) {
            result.push_back(value);
        }
        return result;
    }();
    ASSERT_EQ(values.size(), 6);
    for (std::size_t index = 0; index < values.size(); ++index) {
        EXPECT_EQ(values[index].GetPath(), "outer.array[" + std::to_string(index) + "]");
    }
    EXPECT_EQ(values[1].As<std::string>(), "text");
    EXPECT_TRUE(values[2].IsNull());
    EXPECT_EQ(values[3]["leaf"].GetPath(), "outer.array[3].leaf");
    EXPECT_EQ(values[4][0].GetPath(), "outer.array[4][0]");
}

TEST(FormatsJsonIteratorPaths, CopiedAndMovedStaleIteratorsRestoreCorrectPaths) {
    const auto document = MakeIteratorPathDocument();
    const auto array = document["outer"]["rows"];
    auto source = array.begin();
    const auto old_missing = (*source)["absent"];
    ++source;
    auto copied = source;
    auto moved = std::move(source);
    EXPECT_EQ(copied->GetPath(), "outer.rows[1]");
    EXPECT_EQ(moved->GetPath(), "outer.rows[1]");
    ExpectMissingPath((*copied)["absent"], "outer.rows[1].absent");
    ExpectMissingPath((*moved)["absent"], "outer.rows[1].absent");
    ++copied;
    ++moved;
    EXPECT_TRUE(copied->IsNull());
    EXPECT_TRUE(moved->IsNull());
    EXPECT_EQ(copied->GetPath(), "outer.rows[2]");
    EXPECT_EQ(moved->GetPath(), "outer.rows[2]");
    ExpectMissingPath(old_missing, "outer.rows[0].absent");
}

TEST(FormatsJsonIteratorPaths, CopyAssignmentReplacesRootAndDepthForPaths) {
    const auto document = MakeIteratorPathDocument();
    const auto other = formats::json::FromString(R"({"alternate":{"deeper":{"list":[{},{}]}}})");
    auto target = document["outer"]["rows"].begin();
    EXPECT_EQ(target->GetPath(), "outer.rows[0]");
    ++target;
    auto source = other["alternate"]["deeper"]["list"].begin();
    EXPECT_EQ(source->GetPath(), "alternate.deeper.list[0]");
    ++source;
    target = source;
    EXPECT_EQ(target->GetPath(), "alternate.deeper.list[1]");
    ExpectMissingPath((*target)["absent"], "alternate.deeper.list[1].absent");
}

TEST(FormatsJsonIteratorPaths, MoveAssignmentReplacesRootAndDepthForPaths) {
    const auto document = MakeIteratorPathDocument();
    const auto other = formats::json::FromString(R"({"alternate":{"deeper":{"list":[{},{}]}}})");
    auto target = document["outer"]["rows"].begin();
    EXPECT_EQ(target->GetPath(), "outer.rows[0]");
    ++target;
    auto source = other["alternate"]["deeper"]["list"].begin();
    EXPECT_EQ(source->GetPath(), "alternate.deeper.list[0]");
    ++source;
    target = std::move(source);
    EXPECT_EQ(target->GetPath(), "alternate.deeper.list[1]");
    ExpectMissingPath((*target)["absent"], "alternate.deeper.list[1].absent");
}

TEST(FormatsJsonIteratorPaths, PostincrementReturnsCorrectPathsAndMissingChildren) {
    const auto document = MakeIteratorPathDocument();
    const auto array = document["outer"]["rows"];
    auto it = array.begin();
    for (std::size_t index = 0; index < array.GetSize(); ++index) {
        ASSERT_NE(it, array.end());
        const auto previous = *it++;
        const auto expected = "outer.rows[" + std::to_string(index) + "]";
        EXPECT_EQ(previous.GetPath(), expected);
        ExpectMissingPath(previous["absent"], expected + ".absent");
    }
    EXPECT_EQ(it, array.end());
}

TEST(FormatsJsonIteratorPaths, ItemsRangeReturnsCorrectPaths) {
    const auto document = MakeIteratorPathDocument();
    const auto object = document["outer"]["members"];
    constexpr std::array<std::string_view, 4> names{"first", "second", "nil", "last"};
    std::size_t index = 0;
    for (const auto& [name, value] : Items(object)) {
        ASSERT_LT(index, names.size());
        EXPECT_EQ(name, names[index]);
        const auto expected = "outer.members." + std::string{names[index++]};
        EXPECT_EQ(value.GetPath(), expected);
        ExpectMissingPath(value["absent"], expected + ".absent");
    }
    EXPECT_EQ(index, names.size());
}

TEST(FormatsJsonIteratorPaths, RerootingCopiedValuesDoesNotChangeIteratorPaths) {
    const auto document = MakeIteratorPathDocument();
    const auto array = document["outer"]["rows"];
    for (auto it = array.begin(); it != array.end(); ++it) {
        auto copy = *it;
        const auto expected = "outer.rows[" + std::to_string(it.GetIndex()) + "]";
        copy.DropRootPath();
        EXPECT_EQ(copy.GetPath(), "/");
        ExpectMissingPath(copy["absent"], "absent");
        EXPECT_EQ(it->GetPath(), expected);
        ExpectMissingPath((*it)["absent"], expected + ".absent");
    }
}

TEST(FormatsJsonIteratorPaths, TypeMismatchExceptionPathsFollowEachElement) {
    const auto document = MakeIteratorPathDocument();
    const auto array = document["outer"]["array"];
    std::size_t index = 0;
    for (const auto& value : array) {
        const auto expected = "outer.array[" + std::to_string(index++) + "]";
        SCOPED_TRACE(expected);
        try {
            if (value.IsObject()) {
                value.CheckArray();
            } else {
                value.CheckObject();
            }
            ADD_FAILURE() << "Expected TypeMismatchException for " << expected;
        } catch (const formats::json::TypeMismatchException& exception) {
            EXPECT_EQ(exception.GetPath(), expected);
        }
    }
    EXPECT_EQ(index, 6);
}

USERVER_NAMESPACE_END
