#include <userver/tracing/opentelemetry.hpp>

#include <string>
#include <string_view>

#include <gtest/gtest.h>

USERVER_NAMESPACE_BEGIN

namespace opentelemetry = tracing::opentelemetry;

TEST(OpenTelemetry, TraceParentParsing) {
    const std::string input = "00-80e1afed08e019fc1110464cfa66635c-7a085853722dc6d2-01";

    auto result = opentelemetry::ExtractTraceParentDataView(input);

    ASSERT_TRUE(result.has_value());

    auto data = result.value();

    EXPECT_EQ(data.version, "00");
    EXPECT_EQ(data.trace_id, "80e1afed08e019fc1110464cfa66635c");
    EXPECT_EQ(data.span_id, "7a085853722dc6d2");
    EXPECT_EQ(data.trace_flags, "01");
}

TEST(OpenTelemetry, TraceParentParsingInvalid) {
    const std::vector<std::pair<std::string, std::string>> tests = {
        //"00-80e1afed08e019fc1110464cfa66635c-7a085853722dc6d2-01";
        {"00-80e1afed08e019fc1110464cfa66635c-7a085853722dc6d2-012", "Invalid OpenTelemetry header size"},
        {"00-80e1afed08e019fc1110464cfa66635c-7a085853722dc6d2323",
         "One of the OpenTelemetry header fields has invalid size"},
        {"00-80e1afed08e019fc1110464cfa66635c-7a08585123d2-01-123",
         "One of the OpenTelemetry header fields has invalid size"},
        {"020-80e1afed08e019fc1110464cfa66635c-7a08585372326d2-01", "Invalid version in the OpenTelemetry header"},
        {"00-80e1afed08e019fc1110464cfa66635caa-7a0322dc6d2122-01",
         "One of the OpenTelemetry header fields has invalid size"},
        {"00-80e1afed08e019fc1110464cfa66635c-7a085853722dc1236-1",
         "One of the OpenTelemetry header fields has invalid size"},
        {"00-80e1afed08e019fc1110464cfa66635c-7a085853722dc6d232-",
         "One of the OpenTelemetry header fields has invalid size"},
        {"", "Invalid OpenTelemetry header size"},
        {"---", "Invalid OpenTelemetry header size"},
        {"00-80e1afed08e019fc1110464cfa66635c-7a085853722dc6d2-0100", "Invalid OpenTelemetry header size"},
        {"-------------------------------------------------------", "Invalid version in the OpenTelemetry header"},
        {"00-----------------------------------------------------",
         "One of the OpenTelemetry header fields has invalid size"},
        {"00-80e1afed08e019fc1110464cfa66635z-7a085853722dc6d2--1",
         "One of the OpenTelemetry header fields is not hex data"},
        {"00-80e1afed08e019fc1110464cfa66635c-7a085853722dc6dz-01",
         "One of the OpenTelemetry header fields is not hex data"},
        {"00-80e1afed08e019fc1110464cfa66635c-7a085853722dc6d2-0z",
         "One of the OpenTelemetry header fields is not hex data"},
    };

    for (const auto& [input, expected_error] : tests) {
        auto result = opentelemetry::ExtractTraceParentDataView(input);
        ASSERT_FALSE(result.has_value());
        EXPECT_EQ(result.error(), expected_error) << "For input: " << input;
    }
}

USERVER_NAMESPACE_END
