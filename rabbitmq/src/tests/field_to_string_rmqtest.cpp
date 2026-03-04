#include <amqpcpp.h>

#include <cstdint>
#include <limits>
#include <string>
#include <unordered_map>

#include <userver/utest/utest.hpp>

#include <urabbitmq/impl/field_to_string.hpp>

USERVER_NAMESPACE_BEGIN

UTEST(FieldToString, BasicTypes) {
    AMQP::Table headers;
    headers.set("string", "value");
    headers.set("empty-string", "");
    headers.set("bool-true", true);
    headers.set("bool-false", false);
    headers.set("uint8", static_cast<std::uint8_t>(255));
    headers.set("int8", static_cast<std::int8_t>(-100));
    headers.set("uint16", static_cast<std::uint16_t>(65000));
    headers.set("int16", static_cast<std::int16_t>(-30000));
    headers.set("uint32", std::numeric_limits<std::uint32_t>::max());
    headers.set("int32", std::numeric_limits<std::int32_t>::min());
    headers.set("uint64", std::numeric_limits<std::uint64_t>::max());
    headers.set("int64", std::numeric_limits<std::int64_t>::min());
    headers.set("float", AMQP::Float{3.14f});
    headers.set("double", AMQP::Double{2.718281828});
    headers.set("void", nullptr);

    const std::unordered_map<std::string, std::string> expected_values{
        {"string", "value"},
        {"empty-string", ""},
        {"bool-true", "true"},
        {"bool-false", "false"},
        {"uint8", "255"},
        {"int8", "-100"},
        {"uint16", "65000"},
        {"int16", "-30000"},
        {"uint32", "4294967295"},
        {"int32", "-2147483648"},
        {"uint64", "18446744073709551615"},
        {"int64", "-9223372036854775808"},
        {"float", "3.14"},
        {"double", "2.718281828"},
        {"void", ""},
    };

    ASSERT_EQ(headers.keys().size(), expected_values.size());
    for (const auto& [key, expected_value] : expected_values) {
        ASSERT_TRUE(headers.contains(key)) << "Missing header: " << key;
        EXPECT_EQ(urabbitmq::impl::FieldToString(headers.get(key)), expected_value)
            << "Unexpected converted value for key: " << key;
    }
}

USERVER_NAMESPACE_END
