#include <gmock/gmock.h>
#include <gtest/gtest-typed-test.h>

#include <fmt/format.h>

#include <array>
#include <cstddef>
#include <cstdint>
#include <forward_list>
#include <initializer_list>
#include <iterator>
#include <map>
#include <optional>
#include <ranges>
#include <sstream>
#include <string>
#include <type_traits>
#include <unordered_map>
#include <unordered_set>
#include <utility>
#include <vector>

#include <userver/formats/parse/common_containers.hpp>
#include <userver/formats/serialize/common_containers.hpp>
#include <userver/utils/strong_typedef.hpp>

USERVER_NAMESPACE_BEGIN

namespace {

template <typename Key>
struct SerializationUnsizedMap : std::map<Key, int> {
    void size() const = delete;
};

class SerializationSinglePassRange {
public:
    explicit SerializationSinglePassRange(std::string values)
        : stream_(std::move(values))
    {}

    std::istream_iterator<int> begin() const {
        ++begin_calls_;
        return std::istream_iterator<int>{stream_};
    }

    std::istream_iterator<int> end() const { return {}; }

    std::size_t GetBeginCalls() const { return begin_calls_; }

private:
    mutable std::istringstream stream_;
    mutable std::size_t begin_calls_{0};
};

}  // namespace

template <class Builder>
struct Roundtrip : public ::testing::Test {
    template <typename T>
    void Check(const T& raw_const) {
        // check all ref-cases as we may use different serializers/accessors
        EXPECT_EQ(raw_const, DoRoundtrip(raw_const));
        auto raw_copy = raw_const;
        EXPECT_EQ(raw_const, DoRoundtrip(raw_copy));
        EXPECT_EQ(raw_const, DoRoundtrip(std::move(raw_copy)));
    }

private:
    template <typename T>
    auto DoRoundtrip(T&& raw) {
        return Builder{std::forward<T>(raw)}.ExtractValue().template As<std::decay_t<T>>();
    }
};
TYPED_TEST_SUITE_P(Roundtrip);

TYPED_TEST_P(Roundtrip, Bool) { this->Check(true); }

TYPED_TEST_P(Roundtrip, Int) { this->Check(1); }

TYPED_TEST_P(Roundtrip, Uint64) { this->Check(uint64_t{1}); }

TYPED_TEST_P(Roundtrip, Double) { this->Check(1.0); }

TYPED_TEST_P(Roundtrip, Cstring) {
    // cannot do As<char[]>(), test it manually
    constexpr const char* kTest = "test";
    const auto value = TypeParam{kTest}.ExtractValue();
    EXPECT_EQ(kTest, value.template As<std::string>());
}

TYPED_TEST_P(Roundtrip, String) { this->Check(std::string{"test"}); }

TYPED_TEST_P(Roundtrip, Optional) {
    this->Check(std::optional<int>{});
    this->Check(std::optional<int>{1});
}

TYPED_TEST_P(Roundtrip, Vector) {
    this->Check(std::vector<int>{1, 2, 3});
    this->Check(std::vector<bool>{true, false});
}

TYPED_TEST_P(Roundtrip, SizedAndUnsizedArrays) {
    const std::array values{1, 2, 3};
    const int c_array[]{1, 2, 3};
    const std::forward_list<int> forward_values{1, 2, 3};
    const auto forward_range = std::ranges::subrange(forward_values.begin(), forward_values.end());
    const auto sized_forward_range = std::ranges::subrange(forward_values.begin(), forward_values.end(), values.size());
    const auto expected = testing::ElementsAreArray(values);
    EXPECT_THAT(TypeParam{values}.ExtractValue().template As<std::vector<int>>(), expected);
    EXPECT_THAT(TypeParam{c_array}.ExtractValue().template As<std::vector<int>>(), expected);
    EXPECT_THAT(TypeParam{forward_values}.ExtractValue().template As<std::vector<int>>(), expected);
    EXPECT_THAT(TypeParam{forward_range}.ExtractValue().template As<std::vector<int>>(), expected);
    EXPECT_THAT(TypeParam{sized_forward_range}.ExtractValue().template As<std::vector<int>>(), expected);
}

TYPED_TEST_P(Roundtrip, SinglePassArray) {
    const SerializationSinglePassRange input{"1 2 3"};
    const auto value = TypeParam{input}.ExtractValue();
    EXPECT_THAT(value.template As<std::vector<int>>(), testing::ElementsAre(1, 2, 3));
    EXPECT_EQ(input.GetBeginCalls(), 1);
}

TYPED_TEST_P(Roundtrip, EmptyArrays) {
    const auto sized = TypeParam{std::array<int, 0>{}}.ExtractValue();
    const auto unsized = TypeParam{std::forward_list<int>{}}.ExtractValue();
    EXPECT_TRUE(sized.IsArray());
    EXPECT_TRUE(unsized.IsArray());
    EXPECT_THAT(sized.template As<std::vector<int>>(), testing::IsEmpty());
    EXPECT_THAT(unsized.template As<std::vector<int>>(), testing::IsEmpty());
}

TYPED_TEST_P(Roundtrip, UnorderedMap) { this->Check(std::unordered_map<std::string, int>{{"one", 2}, {"three", 4}}); }

TYPED_TEST_P(Roundtrip, CommonMapSerialization) {
    using Value = decltype(TypeParam{}.ExtractValue());
    for (const std::size_t size : {0, 1, 128}) {
        std::unordered_map<std::string, int> source;
        for (std::size_t index = 0; index < size; ++index) {
            source.emplace(fmt::format("{}", index), index);
        }
        const auto value = formats::serialize::Serialize(source, formats::serialize::To<Value>{});
        EXPECT_TRUE(value.IsObject());
        EXPECT_EQ(value.GetSize(), size);
        EXPECT_EQ((value.template As<decltype(source)>()), source);
        this->Check(source);
    }
}

TYPED_TEST_P(Roundtrip, StrongTypedefMapSerialization) {
    using Key = utils::StrongTypedef<struct MapKeyTag, std::string>;
    using Value = decltype(TypeParam{}.ExtractValue());
    for (const std::size_t size : {0, 1, 128}) {
        std::map<Key, int> source;
        for (std::size_t index = 0; index < size; ++index) {
            source.emplace(Key{fmt::format("{}", index)}, index);
        }
        const auto common = formats::serialize::Serialize(source, formats::serialize::To<Value>{});
        const auto automatic = TypeParam{source}.ExtractValue();
        EXPECT_TRUE(common.IsObject());
        EXPECT_TRUE(automatic.IsObject());
        EXPECT_EQ(common.GetSize(), size);
        EXPECT_EQ(automatic.GetSize(), size);
        for (const auto& [key, expected] : source) {
            EXPECT_EQ(common[key.GetUnderlying()].template As<int>(), expected);
            EXPECT_EQ(automatic[key.GetUnderlying()].template As<int>(), expected);
        }
    }
}

TYPED_TEST_P(Roundtrip, UnsizedMapSerialization) {
    using Key = utils::StrongTypedef<struct UnsizedMapKeyTag, std::string>;
    const auto check = [](const auto& source) {
        static_assert(!std::ranges::sized_range<decltype(source)>);
        using Value = decltype(TypeParam{}.ExtractValue());
        const auto common = formats::serialize::Serialize(source, formats::serialize::To<Value>{});
        const auto automatic = TypeParam{source}.ExtractValue();
        EXPECT_EQ(common.GetSize(), 1);
        EXPECT_EQ(automatic.GetSize(), 1);
        EXPECT_EQ(common["key"].template As<int>(), 42);
        EXPECT_EQ(automatic["key"].template As<int>(), 42);
    };
    SerializationUnsizedMap<std::string> plain;
    plain["key"] = 42;
    check(plain);
    SerializationUnsizedMap<Key> strong;
    strong[Key{"key"}] = 42;
    check(strong);
}

TYPED_TEST_P(Roundtrip, MapWithDuplicateSerializedKeys) {
    const char first[] = "key";
    const char second[] = "key";
    const std::map<const char*, int> source{{first, 1}, {second, 2}};
    using Value = decltype(TypeParam{}.ExtractValue());
    const auto value = formats::serialize::Serialize(source, formats::serialize::To<Value>{});
    EXPECT_EQ(value.GetSize(), 1);
    EXPECT_EQ(value["key"].template As<int>(), source.rbegin()->second);
}

TYPED_TEST_P(Roundtrip, UnorderedSet) { this->Check(std::unordered_set<int>{1, 2, 3}); }

TYPED_TEST_P(Roundtrip, VectorOfStringView) {
    constexpr std::string_view kValue1 = "Some string in std::string_view 1";
    constexpr std::string_view kValue2 = "Some string in std::string_view 2";
    std::vector<std::string_view> data = {kValue1, kValue2};

    // cannot do As<std::vector<std::string_view>>(), test it manually
    const auto extracted = TypeParam{data}.ExtractValue().template As<std::vector<std::string>>();
    EXPECT_THAT(extracted, testing::ElementsAre(kValue1, kValue2));
}

REGISTER_TYPED_TEST_SUITE_P(
    Roundtrip,

    Bool,
    Int,
    Uint64,
    Double,
    Cstring,
    String,
    Optional,
    Vector,
    SizedAndUnsizedArrays,
    SinglePassArray,
    EmptyArrays,
    UnorderedMap,
    CommonMapSerialization,
    StrongTypedefMapSerialization,
    UnsizedMapSerialization,
    MapWithDuplicateSerializedKeys,
    UnorderedSet,
    VectorOfStringView
);

USERVER_NAMESPACE_END
