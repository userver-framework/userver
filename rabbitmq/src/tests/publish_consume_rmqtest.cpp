#include "utils_rmqtest.hpp"

#include <optional>
#include <unordered_map>

#include <amqpcpp.h>

#include <userver/concurrent/variable.hpp>
#include <userver/engine/condition_variable.hpp>
#include <userver/engine/mutex.hpp>
#include <userver/engine/single_consumer_event.hpp>
#include <userver/engine/sleep.hpp>
#include <userver/utils/uuid4.hpp>

USERVER_NAMESPACE_BEGIN

namespace {

class Consumer final : public urabbitmq::ConsumerBase {
public:
    using urabbitmq::ConsumerBase::ConsumerBase;
    ~Consumer() override { Stop(); }

    void Process(urabbitmq::ConsumedMessage message) override {
        const auto plain_message = message.message;
        {
            auto locked = messages_.Lock();
            locked->emplace_back(plain_message);
        }
        {
            auto locked = messages_with_metadata_.Lock();
            locked->emplace_back(std::move(message));
        }

        if (++consumed_ == expected_consumed_) {
            event_.Send();
        }
    }

    void ExpectConsume(size_t count) { expected_consumed_ = count; }

    void Wait() {
        if (expected_consumed_ != 0) {
            [[maybe_unused]] auto res = event_.WaitForEventFor(utest::kMaxTestWaitTime);
        }
    }

    std::vector<std::string> GetMessages() {
        auto locked = messages_.Lock();
        return *locked;
    }

    std::vector<urabbitmq::ConsumedMessage> GetMessagesWithMetadata() {
        auto locked = messages_with_metadata_.Lock();
        return *locked;
    }

private:
    concurrent::Variable<std::vector<std::string>> messages_;
    concurrent::Variable<std::vector<urabbitmq::ConsumedMessage>> messages_with_metadata_;
    std::atomic<size_t> expected_consumed_{0};
    std::atomic<size_t> consumed_{0};
    engine::SingleConsumerEvent event_;
};

class ThrowingConsumer final : public urabbitmq::ConsumerBase {
public:
    using urabbitmq::ConsumerBase::ConsumerBase;
    ~ThrowingConsumer() override { Stop(); }

    void Process(std::string message) override {
        std::unique_lock<engine::Mutex> lock{mutex_};
        if (!thrown_) {
            cond_.WaitFor(lock, utest::kMaxTestWaitTime);
        }

        throw std::runtime_error{message};
    }

    void Throw() {
        std::unique_lock<engine::Mutex> lock{mutex_};
        thrown_ = true;
        cond_.NotifyAll();
    }

private:
    engine::Mutex mutex_;
    bool thrown_{false};
    engine::ConditionVariable cond_;
};

}  // namespace

UTEST(Consumer, CreateOnInvalidQueueWorks) {
    ClientWrapper client{};
    const urabbitmq::ConsumerSettings settings{client.GetQueue(), 10};

    Consumer consumer{client.Get(), settings};
}

UTEST(Consumer, CreateOnInvalidQueueStartStopWorks) {
    ClientWrapper client{};
    const urabbitmq::ConsumerSettings settings{client.GetQueue(), 10};

    Consumer consumer{client.Get(), settings};
    consumer.Start();
    consumer.Stop();
}

UTEST(Consumer, ConsumeWorks) {
    ClientWrapper client{};
    client.SetupRmqEntities();
    const urabbitmq::ConsumerSettings settings{client.GetQueue(), 10};

    const urabbitmq::Envelope envelope{"Hi from userver!", urabbitmq::MessageType::kTransient};
    client->PublishReliable(client.GetExchange(), client.GetRoutingKey(), envelope, client.GetDeadline());

    Consumer consumer{client.Get(), settings};
    consumer.ExpectConsume(1);

    consumer.Start();
    consumer.Wait();
    auto consumed = consumer.GetMessages();

    ASSERT_EQ(consumed.size(), 1);
    EXPECT_EQ(consumed[0], envelope.message);
}

UTEST(Consumer, BasicGetWorks) {
    ClientWrapper client{};
    client.SetupRmqEntities();
    const urabbitmq::ConsumerSettings settings{client.GetQueue(), 10};

    const urabbitmq::Envelope envelope{"Hi from userver!", urabbitmq::MessageType::kTransient};
    client->PublishReliable(client.GetExchange(), client.GetRoutingKey(), envelope, client.GetDeadline());

    const std::string
        consumed_message = client->Get(client.GetQueue(), urabbitmq::Queue::Flags::kNoAck, client.GetDeadline());

    EXPECT_EQ(!consumed_message.empty(), true);
    EXPECT_EQ(consumed_message, envelope.message);
}

UTEST(Consumer, ExhaustesQueue) {
    ClientWrapper client{};
    client.SetupRmqEntities();
    const urabbitmq::ConsumerSettings settings{client.GetQueue(), 10};

    const size_t messages_count = 1000;
    for (size_t i = 0; i < messages_count; ++i) {
        auto channel = client->GetReliableChannel(client.GetDeadline());
        const urabbitmq::Envelope envelope{std::to_string(i), urabbitmq::MessageType::kTransient};
        channel.PublishReliable(client.GetExchange(), client.GetRoutingKey(), envelope, client.GetDeadline());
    }

    Consumer consumer{client.Get(), settings};
    consumer.ExpectConsume(messages_count);
    consumer.Start();

    consumer.Wait();
}

UTEST(Consumer, ThrowsReturnsToQueue) {
    ClientWrapper client{};
    client.SetupRmqEntities();
    const urabbitmq::ConsumerSettings settings{client.GetQueue(), 20};

    const size_t messages_count = 200;
    for (size_t i = 0; i < messages_count; ++i) {
        auto channel = client->GetReliableChannel(client.GetDeadline());
        const urabbitmq::Envelope envelope{std::to_string(i), urabbitmq::MessageType::kTransient};
        channel.PublishReliable(client.GetExchange(), client.GetRoutingKey(), envelope, client.GetDeadline());
    }

    ThrowingConsumer throwing_consumer{client.Get(), settings};
    Consumer good_consumer{client.Get(), settings};
    good_consumer.ExpectConsume(messages_count);
    throwing_consumer.Start();
    good_consumer.Start();
    engine::InterruptibleSleepFor(std::chrono::milliseconds{200});

    auto consumed = good_consumer.GetMessages();
    EXPECT_LT(consumed.size(), messages_count);

    throwing_consumer.Throw();
    throwing_consumer.Stop();
    good_consumer.Wait();
    EXPECT_EQ(good_consumer.GetMessages().size(), messages_count);
}

UTEST(Consumer, MultipleConcurrentWork) {
    ClientWrapper client{};
    client.SetupRmqEntities();
    const urabbitmq::ConsumerSettings settings{client.GetQueue(), 20};

    const size_t messages_count = 1000;
    for (size_t i = 0; i < messages_count; ++i) {
        auto channel = client->GetReliableChannel(client.GetDeadline());
        const urabbitmq::Envelope envelope{std::to_string(i), urabbitmq::MessageType::kTransient};
        channel.PublishReliable(client.GetExchange(), client.GetRoutingKey(), envelope, client.GetDeadline());
    }

    Consumer first_consumer{client.Get(), settings};
    Consumer second_consumer{client.Get(), settings};
    first_consumer.Start();
    second_consumer.Start();

    engine::InterruptibleSleepFor(std::chrono::milliseconds{200});
    EXPECT_GT(first_consumer.GetMessages().size(), 0);
    EXPECT_GT(second_consumer.GetMessages().size(), 0);
}

UTEST(Consumer, ForDifferentQueuesWork) {
    ClientWrapper client{};
    client.SetupRmqEntities();

    const urabbitmq::Queue second_queue{utils::generators::GenerateUuid()};
    {
        auto channel = client->GetAdminChannel(client.GetDeadline());
        channel.DeclareQueue(second_queue, {}, client.GetDeadline());
        channel.BindQueue(client.GetExchange(), second_queue, client.GetRoutingKey(), client.GetDeadline());
    }

    const size_t messages_count = 200;
    for (size_t i = 0; i < messages_count; ++i) {
        const urabbitmq::Envelope envelope{std::to_string(i), urabbitmq::MessageType::kTransient};
        client->PublishReliable(client.GetExchange(), client.GetRoutingKey(), envelope, client.GetDeadline());
    }

    Consumer first_consumer{client.Get(), {client.GetQueue(), 10}};
    Consumer second_consumer{client.Get(), {second_queue, 10}};
    first_consumer.ExpectConsume(messages_count);
    first_consumer.Start();
    second_consumer.ExpectConsume(messages_count);
    second_consumer.Start();

    first_consumer.Wait();
    second_consumer.Wait();
    EXPECT_EQ(first_consumer.GetMessages().size(), messages_count);
    EXPECT_EQ(second_consumer.GetMessages().size(), messages_count);

    client->GetAdminChannel(client.GetDeadline()).RemoveQueue(second_queue, client.GetDeadline());
}

UTEST(Consumer, ConsumeMetadataAndHeadersWork) {
    ClientWrapper client{};
    client.SetupRmqEntities();
    const urabbitmq::ConsumerSettings settings{client.GetQueue(), 10};

    struct Case {
        std::string name;
        std::optional<std::string> reply_to;
        std::optional<std::string> correlation_id;
        std::unordered_map<std::string, std::string> headers;
    };

    const std::vector<Case> cases{
        {"no-user-headers", std::nullopt, std::nullopt, {}},
        {
            "simple-user-headers",
            "reply-queue",
            "corr-id",
            {
                {"x-custom-header", "custom-value"},
                {"x-custom-int", "42"},
            },
        },
        {
            "many-user-headers",
            "reply-many",
            "corr-many",
            {
                {"x-empty", ""},
                {"x-spaces", "a b c"},
                {"x-symbols", R"(!@#$%^&*()[]{}<>/?\\|;:'\",.~-_=+)"},
                {"x-long", std::string(128, 'x')},
            },
        },
        {
            "trace-headers-override",
            "reply-override",
            "corr-override",
            {
                {"u-trace-id", "trace-from-user"},
                {"u-parent-span-id", "parent-from-user"},
                {"x-another", "value"},
            },
        },
    };

    for (const auto& case_data : cases) {
        urabbitmq::Envelope envelope{
            "payload-" + case_data.name,
            urabbitmq::MessageType::kTransient,
        };
        envelope.reply_to = case_data.reply_to;
        envelope.correlation_id = case_data.correlation_id;
        envelope.headers = case_data.headers;
        client->PublishReliable(client.GetExchange(), client.GetRoutingKey(), envelope, client.GetDeadline());
    }

    Consumer consumer{client.Get(), settings};
    consumer.ExpectConsume(cases.size());
    consumer.Start();
    consumer.Wait();
    auto consumed = consumer.GetMessagesWithMetadata();

    ASSERT_EQ(consumed.size(), cases.size());
    std::unordered_map<std::string, const urabbitmq::ConsumedMessage*> consumed_by_payload;
    consumed_by_payload.reserve(consumed.size());
    for (const auto& msg : consumed) {
        consumed_by_payload.emplace(msg.message, &msg);
    }

    for (const auto& case_data : cases) {
        const auto payload = "payload-" + case_data.name;
        const auto it = consumed_by_payload.find(payload);
        ASSERT_NE(it, consumed_by_payload.end()) << "Missing consumed payload: " << payload;

        const auto& msg = *it->second;
        EXPECT_EQ(msg.message, payload);
        EXPECT_EQ(msg.metadata.exchange, client.GetExchange().GetUnderlying());
        EXPECT_EQ(msg.metadata.routingKey, client.GetRoutingKey());
        EXPECT_EQ(msg.reply_to, case_data.reply_to);
        EXPECT_EQ(msg.correlation_id, case_data.correlation_id);

        for (const auto& [header_key, header_value] : case_data.headers) {
            ASSERT_EQ(msg.headers.count(header_key), 1) << "Missing header '" << header_key << "' in " << payload;
            const auto& actual = msg.headers.at(header_key);
            EXPECT_NE(actual.find(header_value), std::string::npos)
                << "Unexpected value for header '" << header_key << "' in " << payload << ": " << actual;
        }

        ASSERT_EQ(msg.headers.count("u-trace-id"), 1) << "Missing u-trace-id in " << payload;
        ASSERT_EQ(msg.headers.count("u-parent-span-id"), 1) << "Missing u-parent-span-id in " << payload;
        EXPECT_FALSE(msg.headers.at("u-trace-id").empty());
        EXPECT_FALSE(msg.headers.at("u-parent-span-id").empty());
    }
}

UTEST(Consumer, HeaderFieldStringConversionInvariants) {
    // rabbitmq/src/urabbitmq/consumer_base_impl.cpp:112
    AMQP::Array array_field;
    array_field.push_back(AMQP::LongString{"arr-string"});
    array_field.push_back(AMQP::Long{123});
    array_field.push_back(AMQP::BooleanSet{true});

    AMQP::Table nested_table;
    nested_table.set("nested-string", "nested-value");
    nested_table.set("nested-int", 7);

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
    headers.set("decimal", AMQP::DecimalField{2, 12345});
    headers.set("void", nullptr);
    headers.set("array", array_field);
    headers.set("table", nested_table);

    for (const auto& key : headers.keys()) {
        EXPECT_NO_THROW({ [[maybe_unused]] const auto value = std::string(headers.get(key)); }
        ) << "Failed for key: "
          << key;
    }
}

USERVER_NAMESPACE_END
