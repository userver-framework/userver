#include <userver/ugrpc/impl/protobuf_collector.hpp>

#include <cstddef>
#include <vector>

#include <gmock/gmock.h>
#include <google/protobuf/stubs/common.h>
#include <gtest/gtest.h>

#include <userver/ugrpc/impl/protobuf_collector.hpp>
#include <userver/ugrpc/protobuf_visit.hpp>

USERVER_NAMESPACE_BEGIN

TEST(GetGeneratedMessages, Ok) {
    const ugrpc::DescriptorList generated_message = ugrpc::impl::GetGeneratedMessages();

    // Please adjust this number if new test proto schemas are added.
#if GOOGLE_PROTOBUF_VERSION >= 4022000
    constexpr std::size_t kExpectedRpcCount = 4;
#else
    constexpr std::size_t kExpectedRpcCount = 3;
#endif

    EXPECT_EQ(generated_message.size(), kExpectedRpcCount * 2);

    EXPECT_THAT(generated_message, testing::Contains(ugrpc::FindGeneratedMessage("sample.ugrpc.GreetingRequest")));
    EXPECT_THAT(generated_message, testing::Contains(ugrpc::FindGeneratedMessage("sample.ugrpc.GreetingResponse")));
    EXPECT_THAT(
        generated_message, testing::Contains(ugrpc::FindGeneratedMessage("sample.ugrpc.StreamGreetingRequest"))
    );
    EXPECT_THAT(
        generated_message, testing::Contains(ugrpc::FindGeneratedMessage("sample.ugrpc.StreamGreetingResponse"))
    );
    EXPECT_THAT(generated_message, testing::Contains(ugrpc::FindGeneratedMessage("google.protobuf.Empty")));
    EXPECT_THAT(
        generated_message, testing::Contains(ugrpc::FindGeneratedMessage("sample.ugrpc.MessageWithDifferentTypes"))
    );
}

USERVER_NAMESPACE_END
