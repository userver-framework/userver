#include <userver/utest/utest.hpp>

#include <ugrpc/impl/protobuf_utils.hpp>

#include <tests/messages.pb.h>

using namespace std::chrono_literals;

USERVER_NAMESPACE_BEGIN

UTEST(ToLimitedString, Fit) {
    std::size_t kLimit = 20;
    sample::ugrpc::GreetingResponse message;
    message.set_name("1234567890");
    const auto out = ugrpc::impl::ToLimitedString(message, kLimit);
    EXPECT_EQ(out, "name: \"1234567890\"\n") << out;
}

UTEST(ToLimitedString, Limited) {
    std::size_t kLimit = 10;
    sample::ugrpc::GreetingResponse message;
    message.set_name("1234567890");
    const auto out = ugrpc::impl::ToLimitedString(message, kLimit);
    EXPECT_EQ(out, "name: \"123") << out;
}

USERVER_NAMESPACE_END
