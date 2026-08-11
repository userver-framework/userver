#include <userver/formats/common/items.hpp>

#include <iterator>
#include <ranges>

#include <gmock/gmock.h>
#include <gtest/gtest.h>
#include <boost/range/adaptor/transformed.hpp>

#include <userver/formats/json/serialize.hpp>
#include <userver/formats/json/value_builder.hpp>
#include <userver/formats/yaml/serialize.hpp>
#include <userver/yaml_config/yaml_config.hpp>

USERVER_NAMESPACE_BEGIN

namespace {

bool IsRvalue(std::string&&) { return true; }

}  // namespace

TEST(FormatsItems, LvalueReference) {
    auto value = formats::json::FromString(R"({"key": "value"})");
    int iterations = 0;
    for (const auto [key, value] : Items(value)) {
        ASSERT_EQ(iterations, 0);
        ASSERT_EQ(key, "key");
        ASSERT_EQ(value.As<std::string>(), "value");

        ++iterations;
    }

    iterations = 0;
    for (auto [key, value] : Items(value)) {
        ASSERT_EQ(iterations, 0);
        ASSERT_EQ(key, "key");
        ASSERT_EQ(value.As<std::string>(), "value");

        ASSERT_TRUE(IsRvalue(std::move(key)));
        ++iterations;
    }
}

TEST(FormatsItems, ConstLvalueReference) {
    /// [Items Example Usage - Simple object]
    const auto value = formats::json::FromString(R"({"key": "value"})");
    int iterations = 0;
    for (const auto& [key, value] : Items(value)) {
        ASSERT_EQ(iterations, 0);
        ASSERT_EQ(key, "key");
        ASSERT_EQ(value.As<std::string>(), "value");

        ++iterations;
    }
    /// [Items Example Usage - Simple object]

    iterations = 0;
    for (auto [key, value] : Items(value)) {
        ASSERT_EQ(iterations, 0);
        ASSERT_EQ(key, "key");
        ASSERT_EQ(value.As<std::string>(), "value");

        ASSERT_TRUE(IsRvalue(std::move(key)));
        ++iterations;
    }
}

TEST(FormatsItems, RvalueReference) {
    auto value = formats::json::FromString(R"({"key": "value"})");
    int iterations = 0;
    for (const auto& [key, value] : Items(std::move(value))) {
        ASSERT_EQ(iterations, 0);
        ASSERT_EQ(key, "key");
        ASSERT_EQ(value.As<std::string>(), "value");

        ++iterations;
    }

    value = formats::json::FromString(R"({"key": "value"})");
    iterations = 0;
    for (auto [key, value] : Items(std::move(value))) {
        ASSERT_EQ(iterations, 0);
        ASSERT_EQ(key, "key");
        ASSERT_EQ(value.As<std::string>(), "value");

        ASSERT_TRUE(IsRvalue(std::move(key)));
    }
}

TEST(FormatsItems, Iterations) {
    auto value = formats::json::FromString(R"({"key1": "v1", "key2": "v2"})");

    auto items = Items(value);
    auto it = items.begin();

    ASSERT_NE(it, items.end());
    auto value_x = *it;
    const std::string key_x = value_x.key;
    auto v_x = value_x.value;

    it++;
    ASSERT_NE(it, items.end());

    auto [key_y, v_y] = *it;

    if (key_x == "key1") {
        ASSERT_EQ(v_x.As<std::string>(), "v1");
        ASSERT_EQ(key_y, "key2");
        ASSERT_EQ(v_y.As<std::string>(), "v2");
    } else {
        ASSERT_EQ(key_x, "key2");
        ASSERT_EQ(v_x.As<std::string>(), "v2");
        ASSERT_EQ(key_y, "key1");
        ASSERT_EQ(v_y.As<std::string>(), "v1");
    }

    ++it;
    ASSERT_EQ(it, items.end());
}

TEST(FormatsItems, IsForwardRange) {
    auto value = formats::json::FromString(R"({"key1": "v1", "key2": "v2"})");

    using MutableItems = decltype(Items(value));
    using ConstItems = decltype(Items(std::as_const(value)));

    static_assert(std::ranges::forward_range<MutableItems>);
    static_assert(std::ranges::forward_range<ConstItems>);

    static_assert(std::forward_iterator<typename MutableItems::iterator>);
    static_assert(std::forward_iterator<typename ConstItems::const_iterator>);
}

TEST(FormatsItems, BoostRanges) {
    auto value = formats::json::FromString(R"({"key1": "v1", "key2": "v2"})");
    EXPECT_THAT(
        Items(value) |
            boost::adaptors::transformed(
                // Must return by value since kv is temporary and stores key.
                [](auto kv) { return std::move(kv.key); }
            ),
        testing::UnorderedElementsAreArray({"key1", "key2"})
    );
    EXPECT_THAT(
        Items(value) |
            boost::adaptors::transformed(
                // Can return by reference since kv only stores value&.
                [](auto kv) -> const auto& { return kv.value; }
            ),
        testing::UnorderedElementsAreArray(
            {formats::json::ValueBuilder{"v1"}.ExtractValue(), formats::json::ValueBuilder{"v2"}.ExtractValue()}
        )
    );
}

TEST(FormatsItems, DocsConstIteration) {
    const auto map = formats::json::FromString(R"({"key": "value"})");
    /// [Items const iteration]
    for (const auto& [name, value] : Items(map)) {
        EXPECT_EQ(name, "key");
        EXPECT_EQ(value.As<std::string>(), "value");
        break;
    }
    /// [Items const iteration]
}

TEST(FormatsItems, DocsMoveValues) {
    auto map = formats::json::FromString(R"({"key": "value"})");
    std::vector<std::string> vector;
    /// [Items move values]
    for (auto [name, value] : Items(map)) {
        vector.push_back(std::move(name));
        // value is a const reference and can not be moved
    }
    /// [Items move values]
    EXPECT_EQ(vector, std::vector<std::string>{"key"});
}

TEST(FormatsItems, DocsConstIterationJson) {
    const auto map = formats::json::FromString(R"({"key": "value"})");
    /// [Items const iteration 2]
    for (const auto& [name, value] : Items(map)) {
        EXPECT_FALSE(name.empty());
        EXPECT_FALSE(value.IsMissing());
        break;
    }
    /// [Items const iteration 2]
}

TEST(FormatsItems, DocsConstIterationYaml) {
    const auto map = formats::yaml::FromString("key: value");
    /// [Items const iteration 3]
    for (const auto& [name, value] : Items(map)) {
        EXPECT_EQ(name, "key");
        break;
    }
    /// [Items const iteration 3]
}

TEST(FormatsItems, DocsConstIterationYamlConfig) {
    const auto map = formats::yaml::FromString("key: value");
    const yaml_config::YamlConfig config(map, {});
    /// [Items const iteration 4]
    for (const auto& [name, value] : Items(config)) {
        EXPECT_EQ(name, "key");
        break;
    }
    /// [Items const iteration 4]
}

USERVER_NAMESPACE_END
