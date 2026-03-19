#include <amqpcpp.h>

#include <cstdint>
#include <string>
#include <unordered_map>
#include <utility>

#include <userver/formats/common/type.hpp>
#include <userver/formats/json/value_builder.hpp>
#include <userver/utest/utest.hpp>

#include <urabbitmq/impl/header_value.hpp>

USERVER_NAMESPACE_BEGIN

namespace {

template <typename T>
formats::json::Value MakeHeaderValue(T&& value) {
    return formats::json::ValueBuilder{std::forward<T>(value)}.ExtractValue();
}

formats::json::Value MakeNestedArrayValue() {
    formats::json::ValueBuilder builder{formats::common::Type::kArray};
    builder.PushBack(std::int64_t{-7});
    builder.PushBack("array-value");

    formats::json::ValueBuilder nested_object{formats::common::Type::kObject};
    nested_object["enabled"] = false;
    nested_object["nullable"] = formats::json::ValueBuilder{};
    builder.PushBack(std::move(nested_object));

    return builder.ExtractValue();
}

formats::json::Value MakeNestedObjectValue() {
    formats::json::ValueBuilder builder{formats::common::Type::kObject};
    builder["count"] = std::uint64_t{42};
    builder["name"] = "nested-object";
    builder["array"] = formats::json::ValueBuilder{MakeNestedArrayValue()};

    return builder.ExtractValue();
}

void ExpectHeadersEqual(
    const std::unordered_map<std::string, urabbitmq::HeaderValue>& actual,
    const std::unordered_map<std::string, urabbitmq::HeaderValue>& expected
) {
    ASSERT_EQ(actual.size(), expected.size());
    for (const auto& [key, expected_value] : expected) {
        const auto it = actual.find(key);
        ASSERT_NE(it, actual.end()) << "Missing key: " << key;
        EXPECT_EQ(it->second, expected_value) << "Unexpected value for key: " << key;
    }
}

}  // namespace

UTEST(HeaderValue, ConvertsNestedAmqpTypes) {
    AMQP::Table headers;
    headers.set("string", "value");
    headers.set("bool", true);
    headers.set("signed", static_cast<std::int16_t>(-10));
    headers.set("unsigned", static_cast<std::uint16_t>(10));
    headers.set("double", AMQP::Double{1.5});
    headers.set("null", nullptr);

    AMQP::Array nested_array;
    nested_array.push_back(AMQP::LongLong{-7});
    nested_array.push_back(AMQP::LongString{"array-value"});
    AMQP::Table nested_array_object;
    nested_array_object.set("enabled", false);
    nested_array_object.set("nullable", nullptr);
    nested_array.push_back(nested_array_object);
    headers.set("array", nested_array);

    AMQP::Table nested_object;
    nested_object.set("count", static_cast<std::uint32_t>(42));
    nested_object.set("name", "nested-object");
    nested_object.set("array", nested_array);
    headers.set("object", nested_object);

    const std::unordered_map<std::string, urabbitmq::HeaderValue> expected{
        {"string", MakeHeaderValue("value")},
        {"bool", MakeHeaderValue(true)},
        {"signed", MakeHeaderValue(std::int64_t{-10})},
        {"unsigned", MakeHeaderValue(std::uint64_t{10})},
        {"double", MakeHeaderValue(1.5)},
        {"null", formats::json::ValueBuilder{}.ExtractValue()},
        {"array", MakeNestedArrayValue()},
        {"object", MakeNestedObjectValue()},
    };

    ExpectHeadersEqual(urabbitmq::impl::TableToHeaders(headers), expected);
}

UTEST(HeaderValue, RoundTripsHeaders) {
    const std::unordered_map<std::string, urabbitmq::HeaderValue> expected{
        {"string", MakeHeaderValue("value")},
        {"bool", MakeHeaderValue(false)},
        {"signed", MakeHeaderValue(std::int64_t{-123456789})},
        {"unsigned", MakeHeaderValue(std::uint64_t{123456789})},
        {"double", MakeHeaderValue(3.25)},
        {"null", formats::json::ValueBuilder{}.ExtractValue()},
        {"array", MakeNestedArrayValue()},
        {"object", MakeNestedObjectValue()},
    };

    AMQP::Table table;
    urabbitmq::impl::AddHeadersToTable(table, expected);

    ExpectHeadersEqual(urabbitmq::impl::TableToHeaders(table), expected);
}

USERVER_NAMESPACE_END
