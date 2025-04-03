#include <userver/dump/protobuf.hpp>

#include <gmock/gmock.h>
#include <google/protobuf/util/json_util.h>
#include <google/protobuf/util/message_differencer.h>

#include <userver/dump/common_containers.hpp>
#include <userver/dump/test_helpers.hpp>
#include <userver/ugrpc/impl/to_string.hpp>
#include <userver/utest/utest.hpp>

#include <tests/messages.pb.h>

USERVER_NAMESPACE_BEGIN

namespace {

std::string AsJsonString(const google::protobuf::Message& message) {
    grpc::string output;
    const auto status = google::protobuf::util::MessageToJsonString(message, &output);
    if (!status.ok()) {
        throw std::runtime_error(status.ToString());
    }
    return ugrpc::impl::ToString(output);
}

void ExpectEquals(const google::protobuf::Message& message1, const google::protobuf::Message& message2) {
    EXPECT_TRUE(google::protobuf::util::MessageDifferencer::Equals(message1, message2))
        << AsJsonString(message1) << " != " << AsJsonString(message2);
}

template <typename Message>
void TestWriteReadCycle(const Message& message) {
    const auto after_cycle = dump::FromBinary<Message>(dump::ToBinary(message));
    ExpectEquals(after_cycle, message);
}

sample::ugrpc::GreetingResponse MakeFilledMessage() {
    sample::ugrpc::GreetingResponse filled_message;
    filled_message.set_name("userver");
    filled_message.set_greeting("Hi");
    return filled_message;
}

}  // namespace

TEST(DumpProtobuf, Empty) { TestWriteReadCycle(sample::ugrpc::GreetingResponse{}); }

TEST(DumpProtobuf, Flat) { TestWriteReadCycle(MakeFilledMessage()); }

TEST(DumpProtobuf, Vector) {
    const std::vector source{
        sample::ugrpc::GreetingResponse{},
        MakeFilledMessage(),
    };

    const auto after_cycle = dump::FromBinary<decltype(source)>(dump::ToBinary(source));

    ASSERT_THAT(after_cycle, testing::SizeIs(2));
    ExpectEquals(after_cycle[0], source[0]);
    ExpectEquals(after_cycle[1], source[1]);
}

TEST(DumpProtobuf, InterspersedWithOtherContent) {
    const std::unordered_map<std::string, std::pair<std::int64_t, sample::ugrpc::GreetingResponse>> source{
        {"foo", {10, MakeFilledMessage()}},
        {"bar", {1000000000000000000LL, sample::ugrpc::GreetingResponse{}}},
        {"qux", {1000, MakeFilledMessage()}},
    };

    const auto after_cycle = dump::FromBinary<decltype(source)>(dump::ToBinary(source));

    ASSERT_THAT(after_cycle, testing::SizeIs(3));
    for (const auto key : {"foo", "bar", "qux"}) {
        ASSERT_EQ(after_cycle.count(key), 1);
        EXPECT_EQ(after_cycle.at(key).first, source.at(key).first);
        ExpectEquals(after_cycle.at(key).second, source.at(key).second);
    }
}

USERVER_NAMESPACE_END
