#include <userver/utest/utest.hpp>

#include <gmock/gmock.h>

#include <userver/ugrpc/protobuf_logging.hpp>

#include <tests/logging.pb.h>
#include <tests/messages.pb.h>

USERVER_NAMESPACE_BEGIN

namespace {

constexpr std::string_view kTruncateMarker = "...(truncated)";

sample::ugrpc::LoggingMessage ConstructComplexMessage() {
    sample::ugrpc::LoggingMessage message;

    message.set_id("test-id");

    for (int i = 0; i < 10; ++i) {
        *message.add_names() = "test-name-" + std::to_string(i);
    }

    for (int i = 0; i < 10; ++i) {
        auto* item = message.add_items();
        item->set_index(i);
        item->set_value("test-value-" + std::to_string(i));
    }

    for (int i = 0; i < 10; ++i) {
        (*message.mutable_properties()
        )["test-property-name-" + std::to_string(i)] = "test-property-" + std::to_string(i);
    }

    return message;
}

}  // namespace

UTEST(ToLimitedLoggingString, Basic) {
    constexpr std::size_t kLimit = 200;
    sample::ugrpc::LoggingMessage message;

    message.set_id("swag");
    *message.add_names() = "test-name-1";
    *message.add_names() = "test-name-2";
    auto out = ugrpc::ToLimitedLoggingString(message, kLimit);
    EXPECT_EQ(out, R"({"id":"swag","names":["test-name-1","test-name-2"]})");

    out = ugrpc::ToLimitedLoggingString(message, 20);
    EXPECT_THAT(out, testing::HasSubstr(R"("id":"swag")"));
    EXPECT_THAT(out, testing::EndsWith(kTruncateMarker));
}

UTEST(ToLimitedLoggingString, Fit) {
    sample::ugrpc::GreetingResponse message;
    message.set_name("1234567890");
    const auto out = ugrpc::ToLimitedLoggingString(message, 25);
    EXPECT_EQ(out, R"({"name":"1234567890"})");
}

UTEST(ToLimitedLoggingString, Limited) {
    constexpr std::size_t kLimit = 30;
    sample::ugrpc::GreetingResponse message;
    message.set_name("123456789012345678901234567890");
    const auto out = ugrpc::ToLimitedLoggingString(message, kLimit);
    EXPECT_THAT(out, testing::HasSubstr(R"("name")"));
    EXPECT_THAT(out, testing::EndsWith(kTruncateMarker));
}

UTEST(ToLimitedLoggingString, EdgeCaseTruncateToMarker) {
    constexpr std::size_t kLimit = kTruncateMarker.size();
    sample::ugrpc::GreetingResponse message;
    message.set_name("12345678901234567890");
    {
        const auto out = ugrpc::ToLimitedLoggingString(message, kLimit);
        EXPECT_THAT(out, testing::EndsWith(kTruncateMarker));
    }
    {
        const auto out = ugrpc::ToLimitedLoggingString(message, kLimit + 1);
        EXPECT_THAT(out, testing::EndsWith(kTruncateMarker));
    }
}

UTEST(ToLimitedLoggingString, EdgeCaseTruncateUpToMarker) {
    constexpr std::size_t kLimit = kTruncateMarker.size();
    sample::ugrpc::GreetingResponse message;
    message.set_name("1");

    const auto expected = R"({"name":"1"})";
    {
        const auto out = ugrpc::ToLimitedLoggingString(message, kLimit);
        EXPECT_EQ(out, expected);
    }
    {
        const auto out = ugrpc::ToLimitedLoggingString(message, kLimit + 1);
        EXPECT_EQ(out, expected);
    }
}

UTEST(ToLimitedLoggingString, EdgeCaseFullyTruncated) {
    constexpr std::size_t kLimit = 0;
    sample::ugrpc::GreetingResponse message;
    message.set_name("12345678901234567890");
    const auto out = ugrpc::ToLimitedLoggingString(message, kLimit);
    EXPECT_EQ(out, kTruncateMarker);
}

UTEST(ToLimitedLoggingString, EdgeCaseLimitOne) {
    constexpr std::size_t kLimit = 1;
    sample::ugrpc::GreetingResponse message;
    message.set_name("12345678901234567890");
    const auto out = ugrpc::ToLimitedLoggingString(message, kLimit);
    EXPECT_THAT(out, testing::EndsWith(kTruncateMarker));
}

UTEST(ToLimitedLoggingString, EdgeCaseSeven) {
    // With limit 7 the JSON cannot be completed, so the truncation marker is appended.
    constexpr std::size_t kLimit = 7;

    sample::ugrpc::GreetingResponse message;
    message.set_name("12345678901234567890");
    const auto out = ugrpc::ToLimitedLoggingString(message, kLimit);
    EXPECT_THAT(out, testing::EndsWith(kTruncateMarker));
}

UTEST(ToLimitedLoggingString, Complex) {
    constexpr std::size_t kLimit = 512;
    const auto message = ConstructComplexMessage();
    const auto out = ugrpc::ToLimitedLoggingString(message, kLimit);
    EXPECT_THAT(out, testing::EndsWith(kTruncateMarker));
    EXPECT_THAT(out, testing::HasSubstr(R"("id":"test-id")"));
    EXPECT_THAT(out, testing::HasSubstr("test-name-0"));
    EXPECT_THAT(out, testing::HasSubstr("test-value-0"));
}

USERVER_NAMESPACE_END
