#include <userver/ugrpc/impl/protobuf_collector.hpp>

#include <cstddef>
#include <vector>

#include <gmock/gmock.h>
#include <gtest/gtest.h>

#include <userver/ugrpc/impl/protobuf_collector.hpp>
#include <userver/ugrpc/protobuf_visit.hpp>

USERVER_NAMESPACE_BEGIN

TEST(GetGeneratedMessages, Ok) {
  const ugrpc::DescriptorList generated_message =
      ugrpc::impl::GetGeneratedMessages();

  // Currently, there are 8 different request/response types in the binary.
  // You should adjust this number if you start using new types
  // as requests/responses in the tests' protobufs
  EXPECT_EQ(generated_message.size(), 8);

  EXPECT_THAT(generated_message, testing::Contains(ugrpc::FindGeneratedMessage(
                                     "sample.ugrpc.SendRequest")));
  EXPECT_THAT(generated_message, testing::Contains(ugrpc::FindGeneratedMessage(
                                     "sample.ugrpc.SendResponse")));
  EXPECT_THAT(generated_message, testing::Contains(ugrpc::FindGeneratedMessage(
                                     "sample.ugrpc.GreetingRequest")));
  EXPECT_THAT(generated_message, testing::Contains(ugrpc::FindGeneratedMessage(
                                     "sample.ugrpc.GreetingResponse")));
  EXPECT_THAT(generated_message, testing::Contains(ugrpc::FindGeneratedMessage(
                                     "sample.ugrpc.StreamGreetingRequest")));
  EXPECT_THAT(generated_message, testing::Contains(ugrpc::FindGeneratedMessage(
                                     "sample.ugrpc.StreamGreetingResponse")));
  EXPECT_THAT(
      generated_message,
      testing::Contains(ugrpc::FindGeneratedMessage("google.protobuf.Empty")));
  EXPECT_THAT(generated_message,
              testing::Contains(ugrpc::FindGeneratedMessage(
                  "sample.ugrpc.MessageWithDifferentTypes")));
}

USERVER_NAMESPACE_END
