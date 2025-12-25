#include <userver/utest/utest.hpp>

#include <userver/ugrpc/impl/to_string.hpp>
#include <userver/ugrpc/protobuf_logging.hpp>

#include <tests/logging.pb.h>
#include <tests/messages.pb.h>

using namespace std::chrono_literals;

USERVER_NAMESPACE_BEGIN

namespace {

const std::string kNewLineTruncatedMarker = "\n...(truncated)";
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

UTEST(ToLimitedDebugString, Basic) {
    constexpr std::size_t kLimit = 200;
    sample::ugrpc::LoggingMessage message;

    message.set_id("swag");
    *message.add_names() = "test-name-1";
    *message.add_names() = "test-name-2";
    auto out = ugrpc::ToLimitedDebugString(message, kLimit);
    const auto expected = message.DebugString();
    EXPECT_EQ(out, expected);

    out = ugrpc::ToLimitedDebugString(message, 20);
    EXPECT_EQ(out, expected.substr(0, 5) + kNewLineTruncatedMarker);
}

UTEST(ToLimitedDebugString, Fit) {
    constexpr std::size_t kLimit = 20;
    sample::ugrpc::GreetingResponse message;
    message.set_name("1234567890");
    const auto out = ugrpc::ToLimitedDebugString(message, kLimit);
    EXPECT_EQ(out, "name: \"1234567890\"\n");
}

UTEST(ToLimitedDebugString, Limited) {
    constexpr std::size_t kLimit = 30;
    sample::ugrpc::GreetingResponse message;
    message.set_name("123456789012345678901234567890");
    const auto out = ugrpc::ToLimitedDebugString(message, kLimit);
    EXPECT_EQ(out, "name: \"12345678\n...(truncated)");
}

UTEST(ToLimitedDebugString, EdgeCaseTruncateToMarker) {
    constexpr std::size_t kLimit = kTruncateMarker.size();
    sample::ugrpc::GreetingResponse message;
    message.set_name("12345678901234567890");
    {
        const auto out = ugrpc::ToLimitedDebugString(message, kLimit);
        EXPECT_EQ(out, kTruncateMarker);
    }
    {
        const auto out = ugrpc::ToLimitedDebugString(message, kLimit + 1);
        EXPECT_EQ(out, kTruncateMarker);
    }
}

UTEST(ToLimitedDebugString, EdgeCaseTruncateUpToMarker) {
    constexpr std::size_t kLimit = kTruncateMarker.size();
    sample::ugrpc::GreetingResponse message;
    message.set_name("1");

    const auto expected = "name: \"1\"\n";
    {
        const auto out = ugrpc::ToLimitedDebugString(message, kLimit);
        EXPECT_EQ(out, expected);
    }
    {
        const auto out = ugrpc::ToLimitedDebugString(message, kLimit + 1);
        EXPECT_EQ(out, expected);
    }
}

UTEST(ToLimitedDebugString, EdgeCaseFullyTruncated) {
    constexpr std::size_t kLimit = 0;
    sample::ugrpc::GreetingResponse message;
    message.set_name("12345678901234567890");
    const auto out = ugrpc::ToLimitedDebugString(message, kLimit);
    EXPECT_EQ(out, kTruncateMarker);
}

UTEST(ToLimitedDebugString, EdgeCaseLimitOne) {
    constexpr std::size_t kLimit = 1;
    sample::ugrpc::GreetingResponse message;
    message.set_name("12345678901234567890");
    const auto out = ugrpc::ToLimitedDebugString(message, kLimit);
    EXPECT_EQ(out, kTruncateMarker);
}

UTEST(ToLimitedDebugString, EdgeCaseSeven) {
    // <EMPTY> will fit, but there should be "...", because string is not truncated to empty
    constexpr std::size_t kLimit = 7;

    sample::ugrpc::GreetingResponse message;
    message.set_name("12345678901234567890");
    const auto out = ugrpc::ToLimitedDebugString(message, kLimit);
    EXPECT_EQ(out, kTruncateMarker);
}

UTEST(ToLimitedDebugString, Complex) {
    constexpr std::size_t kLimit = 512;
    const auto message = ConstructComplexMessage();
    const auto expected =
        ugrpc::impl::ToString(message.Utf8DebugString().substr(0, kLimit - kNewLineTruncatedMarker.size())) +
        kNewLineTruncatedMarker;
    ASSERT_EQ(expected, ugrpc::ToLimitedDebugString(message, kLimit));
}

USERVER_NAMESPACE_END
