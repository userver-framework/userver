#include <userver/utest/utest.hpp>

#include <userver/ugrpc/impl/to_string.hpp>

#include <ugrpc/impl/protobuf_utils.hpp>

#include <tests/logging.pb.h>
#include <tests/messages.pb.h>

using namespace std::chrono_literals;

USERVER_NAMESPACE_BEGIN

namespace {

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
        (*message.mutable_properties())["test-property-name-" + std::to_string(i)] =
            "test-property-" + std::to_string(i);
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
    auto out = ugrpc::impl::ToLimitedDebugString(message, kLimit);
    const auto expected = message.DebugString();
    EXPECT_EQ(out, expected);

    out = ugrpc::impl::ToLimitedDebugString(message, 8);
    EXPECT_EQ(out, expected.substr(0, 8));
}

UTEST(ToLimitedDebugString, Fit) {
    constexpr std::size_t kLimit = 20;
    sample::ugrpc::GreetingResponse message;
    message.set_name("1234567890");
    const auto out = ugrpc::impl::ToLimitedDebugString(message, kLimit);
    EXPECT_EQ(out, "name: \"1234567890\"\n");
}

UTEST(ToLimitedDebugString, Limited) {
    constexpr std::size_t kLimit = 10;
    sample::ugrpc::GreetingResponse message;
    message.set_name("1234567890");
    const auto out = ugrpc::impl::ToLimitedDebugString(message, kLimit);
    EXPECT_EQ(out, "name: \"123");
}

UTEST(ToLimitedDebugString, Complex) {
    constexpr std::size_t kLimit = 512;
    const auto message = ConstructComplexMessage();
    const auto expected = ugrpc::impl::ToString(message.Utf8DebugString().substr(0, kLimit));
    ASSERT_EQ(expected, ugrpc::impl::ToLimitedDebugString(message, kLimit));
}

USERVER_NAMESPACE_END
