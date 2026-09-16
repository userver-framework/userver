#include "utils_rmqtest.hpp"

#include <string>
#include <unordered_map>

#include <userver/formats/json/value_builder.hpp>

USERVER_NAMESPACE_BEGIN

UTEST(AdminChannel, DeclareRemoveExchange) {
    ClientWrapper client{};
    auto channel = client->GetAdminChannel(client.GetDeadline());
    urabbitmq::Exchange exchange{"some_exchange"};

    const auto declare_exchange =
        [&client, &exchange](urabbitmq::AdminChannel& channel, urabbitmq::Exchange::Type type) {
            channel.DeclareExchange(exchange, type, {}, client.GetDeadline());
        };
    declare_exchange(channel, urabbitmq::Exchange::Type::kFanOut);
    // 406 PRECONDITION_FAILED
    EXPECT_ANY_THROW(declare_exchange(channel, urabbitmq::Exchange::Type::kDirect));

    // channel is broken
    EXPECT_ANY_THROW(channel.RemoveExchange(exchange, client.GetDeadline()));

    auto new_channel = client->GetAdminChannel(client.GetDeadline());
    new_channel.RemoveExchange(exchange, client.GetDeadline());
    declare_exchange(new_channel, urabbitmq::Exchange::Type::kFanOut);
    new_channel.RemoveExchange(exchange, client.GetDeadline());
}

UTEST(AdminChannel, DeclareRemoveQueue) {
    ClientWrapper client{};
    auto channel = client->GetAdminChannel(client.GetDeadline());
    urabbitmq::Queue queue{"some_queue"};

    const auto declare_queue =
        [&client, &queue](urabbitmq::AdminChannel& channel, utils::Flags<urabbitmq::Queue::Flags> flags) {
            channel.DeclareQueue(queue, flags, client.GetDeadline());
        };
    declare_queue(channel, urabbitmq::Queue::Flags::kDurable);
    // 406 PRECONDITION_FAILED
    EXPECT_ANY_THROW(declare_queue(channel, urabbitmq::Queue::Flags::kNone));

    // channel is broken
    EXPECT_ANY_THROW(channel.RemoveQueue(queue, client.GetDeadline()));

    auto new_channel = client->GetAdminChannel(client.GetDeadline());
    new_channel.RemoveQueue(queue, client.GetDeadline());
    declare_queue(new_channel, urabbitmq::Queue::Flags::kNone);
    new_channel.RemoveQueue(queue, client.GetDeadline());
}

UTEST(AdminChannel, DeclareQueueWithMaxLengthArgument) {
    // This test checks, whether optional args for queue are working
    ClientWrapper client{};
    auto channel = client->GetAdminChannel(client.GetDeadline());

    constexpr std::int64_t kMaxLength = 3;
    const std::unordered_map<std::string, urabbitmq::HeaderValue> arguments{
        {"x-max-length", urabbitmq::HeaderValue::Builder{std::int64_t{kMaxLength}}.ExtractValue()},
    };

    channel.DeclareExchange(client.GetExchange(), urabbitmq::Exchange::Type::kFanOut, {}, client.GetDeadline());
    channel.DeclareQueue(client.GetQueue(), {}, arguments, client.GetDeadline());
    channel.BindQueue(client.GetExchange(), client.GetQueue(), client.GetRoutingKey(), client.GetDeadline());

    constexpr int kPublishCount = kMaxLength + 10;
    for (int i = 0; i < kPublishCount; ++i) {
        client->PublishReliable(
            client.GetExchange(), client.GetRoutingKey(), "message-" + std::to_string(i), client.GetDeadline()
        );
    }
    const auto response = channel.DeclareQueue(client.GetQueue(), {}, arguments, client.GetDeadline());
    EXPECT_EQ(response.message_count, kMaxLength);

    // The default (drop-head) overflow drops the oldest messages, so the queue
    // keeps only the newest kMaxLength ones. Reading them back in FIFO order must
    // yield that tail: message-10, message-11, message-12.
    for (int i = 0; i < kMaxLength; ++i) {
        const auto message = client->Get(client.GetQueue(), urabbitmq::Queue::Flags::kNoAck, client.GetDeadline());
        EXPECT_EQ(message, "message-" + std::to_string(kPublishCount - kMaxLength + i));
    }
}

UTEST(AdminChannel, DeclareQueueArgumentMismatchFails) {
    // Here we are testing the same queue with different optional args
    ClientWrapper client{};

    const auto declare_with_max_length = [&client](std::int64_t max_length) {
        // A fresh channel per attempt: a PRECONDITION_FAILED breaks the channel
        // it happens on, so reusing one would mask the cause of later failures.
        auto channel = client->GetAdminChannel(client.GetDeadline());
        const std::unordered_map<std::string, urabbitmq::HeaderValue> arguments{
            {"x-max-length", urabbitmq::HeaderValue::Builder{std::int64_t{max_length}}.ExtractValue()},
        };
        channel.DeclareQueue(client.GetQueue(), {}, arguments, client.GetDeadline());
    };

    // First declaration creates the queue with x-max-length == 3.
    declare_with_max_length(3);

    // Re-declaring the same queue with a different argument value is a conflict.
    EXPECT_ANY_THROW(declare_with_max_length(5));

    // Re-declaring with the very same value is idempotent and must not throw.
    EXPECT_NO_THROW(declare_with_max_length(3));
}

USERVER_NAMESPACE_END
