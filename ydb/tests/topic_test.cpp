#include <userver/utest/utest.hpp>

#include <userver/engine/async.hpp>
#include <userver/utils/overloaded.hpp>
#include <userver/ydb/impl/cast.hpp>

#include "test_utils.hpp"

USERVER_NAMESPACE_BEGIN

namespace {

constexpr std::string_view kTable = "test_table";
constexpr std::string_view kChangefeed = "test_changefeed";
const std::string kTopicPath = fmt::format("{}/{}", kTable, kChangefeed);
constexpr std::string_view kConsumerName = "test_consumer";

constexpr std::string_view kWriteTopic = "write_test_topic";
constexpr std::string_view kWriteProducerId = "test-producer";
constexpr std::string_view kWriteConsumerName = "write_test_consumer";

class YdbTopicFixture : public ydb::ClientFixtureBase {
protected:
    YdbTopicFixture() {
        CreateTable(kTable);
        AddChangefeed(kTable, kChangefeed);
    }

    void CreateTable(std::string_view table) {
        DoCreateTable(
            table,
            NYdb::NTable::TTableBuilder()
                .AddNullableColumn("key", NYdb::EPrimitiveType::Uint32)
                .AddNullableColumn("value", NYdb::EPrimitiveType::String)
                .SetPrimaryKeyColumn("key")
                .Build()
        );
    }

    void AddChangefeed(std::string_view table, std::string_view changefeed) {
        GetTableClient().ExecuteSchemeQuery(fmt::format(
            R"-(
                    ALTER TABLE `{}` ADD CHANGEFEED `{}` WITH (
                        MODE = 'NEW_AND_OLD_IMAGES',
                        FORMAT = 'JSON'
                    );
                )-",
            table,
            changefeed
        ));
    }

    void DropChangefeed(std::string_view table, std::string_view changefeed) {
        GetTableClient().ExecuteSchemeQuery(fmt::format("ALTER TABLE `{}` DROP CHANGEFEED `{}`;", table, changefeed));
    }

    void AddConsumer(std::string_view topic_path, std::string_view consumer_name) {
        NYdb::NTopic::TAlterTopicSettings settings;
        settings.AppendAddConsumers({settings, ydb::impl::ToString(consumer_name)});
        AlterTopic(topic_path, settings);
    }

    void DropConsumer(std::string_view topic_path, std::string_view consumer_name) {
        NYdb::NTopic::TAlterTopicSettings settings;
        settings.AppendDropConsumers(ydb::impl::ToString(consumer_name));
        AlterTopic(topic_path, settings);
    }

    void AlterTopic(std::string_view topic_path, const NYdb::NTopic::TAlterTopicSettings& settings) {
        const auto status = GetNativeTopicClient().AlterTopic(ydb::impl::ToString(topic_path), settings).GetValueSync();
        ASSERT_TRUE(status.IsSuccess()) << "AlterTopic failed: " + status.GetIssues().ToString();
    }

    ydb::TopicReadSession CreateReadSession(std::string_view topic_path, std::string_view consumer_name) {
        NYdb::NTopic::TReadSessionSettings read_session_settings;
        read_session_settings.AppendTopics(ydb::impl::ToString(topic_path));
        read_session_settings.ConsumerName(ydb::impl::ToString(consumer_name));
        return GetTopicClient().CreateReadSession(read_session_settings);
    }

    ydb::TopicWriteSession CreateWriteSession(std::string_view topic_path, std::string_view producer_id) {
        const auto producer = ydb::impl::ToString(producer_id);
        NYdb::NTopic::TWriteSessionSettings write_session_settings;
        write_session_settings.Path(ydb::impl::ToString(topic_path)).ProducerId(producer).MessageGroupId(producer);
        return GetTopicClient().CreateWriteSession(write_session_settings);
    }

    void WriteAndAck(ydb::TopicWriteSession& session, std::string_view payload) {
        bool written = false;
        bool acked = false;

        while (!written || !acked) {
            auto event = session.GetEvent();

            std::visit(
                utils::Overloaded{
                    [&](NYdb::NTopic::TWriteSessionEvent::TReadyToAcceptEvent& e) {
                        session
                            .Write(std::move(e.ContinuationToken), NYdb::NTopic::TWriteMessage{std::string{payload}});
                        written = true;
                    },
                    [&](NYdb::NTopic::TWriteSessionEvent::TAcksEvent& e) {
                        for (const auto& ack : e.Acks) {
                            EXPECT_EQ(ack.State, NYdb::NTopic::TWriteSessionEvent::TWriteAck::EEventState::EES_WRITTEN);
                        }
                        acked = true;
                    },
                    [](const NYdb::NTopic::TSessionClosedEvent& e) {
                        FAIL() << "Session closed unexpectedly: " << e.GetIssues().ToString();
                    },
                    []([[maybe_unused]] auto& e) {},
                },
                event
            );
        }
    }
};

class YdbTopicWriteSessionFixture : public YdbTopicFixture {
protected:
    YdbTopicWriteSessionFixture() {
        NYdb::NTopic::TCreateTopicSettings topic_settings;
        topic_settings
            .AppendConsumers(NYdb::NTopic::TConsumerSettings(topic_settings, ydb::impl::ToString(kWriteConsumerName)));
        const auto status =
            GetNativeTopicClient().CreateTopic(ydb::impl::ToString(kWriteTopic), topic_settings).GetValueSync();
        EXPECT_TRUE(status.IsSuccess()) << status.GetIssues().ToString();
    }

    ~YdbTopicWriteSessionFixture() override {
        const auto status = GetNativeTopicClient().DropTopic(ydb::impl::ToString(kWriteTopic)).GetValueSync();
        EXPECT_TRUE(status.IsSuccess()) << status.GetIssues().ToString();
    }

    ydb::TopicWriteSession CreateWriteSession() {
        return YdbTopicFixture::CreateWriteSession(kWriteTopic, kWriteProducerId);
    }

    ydb::TopicReadSession CreateReadSession() {
        return YdbTopicFixture::CreateReadSession(kWriteTopic, kWriteConsumerName);
    }
};

template <class T>
class YdbTopicReadSessionWithDataHandler : public YdbTopicFixture {
protected:
    T CreateReadSessionWithDataHandler(
        std::string_view topic_path,
        std::string_view consumer_name,
        std::function<void()> data_handler = []() {},
        bool commit_data_events = false
    );
};

template <>
ydb::TopicReadSession YdbTopicReadSessionWithDataHandler<ydb::TopicReadSession>::CreateReadSessionWithDataHandler(
    std::string_view topic_path,
    std::string_view consumer_name,
    std::function<void()> data_handler,
    bool commit_data_events
) {
    NYdb::NTopic::TReadSessionSettings read_session_settings;
    read_session_settings.AppendTopics(ydb::impl::ToString(topic_path));
    read_session_settings.ConsumerName(ydb::impl::ToString(consumer_name));
    read_session_settings.EventHandlers_
        .SimpleDataHandlers([handler = std::move(data_handler)](const auto&) { handler(); }, commit_data_events);
    return GetTopicClient().CreateReadSession(read_session_settings);
}

template <>
ydb::FederatedTopicReadSession
YdbTopicReadSessionWithDataHandler<ydb::FederatedTopicReadSession>::CreateReadSessionWithDataHandler(
    std::string_view topic_path,
    std::string_view consumer_name,
    std::function<void()> data_handler,
    bool commit_data_events
) {
    NYdb::NFederatedTopic::TFederatedReadSessionSettings read_session_settings;
    read_session_settings.AppendTopics(ydb::impl::ToString(topic_path));
    read_session_settings.ConsumerName(ydb::impl::ToString(consumer_name));
    read_session_settings.FederatedEventHandlers_
        .SimpleDataHandlers([handler = std::move(data_handler)](const auto&) { handler(); }, commit_data_events);

    return GetFederatedTopicClient().CreateReadSession(read_session_settings);
}

}  // namespace

using ReadSessionTypes = ::testing::Types<ydb::TopicReadSession, ydb::FederatedTopicReadSession>;
TYPED_UTEST_SUITE(YdbTopicReadSessionWithDataHandler, ReadSessionTypes);

TYPED_UTEST(YdbTopicReadSessionWithDataHandler, TopicReadSessionCreateClose) {
    this->AddConsumer(kTopicPath, kConsumerName);
    auto session = this->CreateReadSessionWithDataHandler(kTopicPath, kConsumerName);
    UASSERT_NO_THROW(session.Close(std::chrono::milliseconds{1000}));
    this->DropConsumer(kTopicPath, kConsumerName);
}

TYPED_UTEST(YdbTopicReadSessionWithDataHandler, CommitDataEventsPersistence) {
    this->AddConsumer(kTopicPath, kConsumerName);

    this->GetTableClient().ExecuteDataQuery(fmt::format(
        R"-(
      INSERT INTO {} (key, value)
      VALUES
        (123, "qwe"),
        (321, "xyz");
    )-",
        kTable
    ));

    auto read_all = [&](bool commit_data_event) {
        std::atomic<size_t> data_events_count = 0;
        TypeParam session = this->CreateReadSessionWithDataHandler(
            kTopicPath,
            kConsumerName,
            [&data_events_count]() { data_events_count++; },
            commit_data_event
        );

        auto task = engine::AsyncNoTracing([&session] {
            UASSERT_NO_THROW(session.GetNativeTopicReadSession().WaitEvent().Wait(std::chrono::milliseconds{1000}));
        });
        task.WaitFor(std::chrono::milliseconds{1000});
        Y_ENSURE(task.IsFinished());

        session.Close(std::chrono::milliseconds{1000});
        return data_events_count.load();
    };

    // Read entire topic without committing events, leaving consumer's offset unchanged
    size_t count = read_all(false);
    ASSERT_TRUE(count > 0u);

    // Read entire topic once again and commit each event, changing consumer's offset
    count = read_all(true);
    ASSERT_TRUE(count > 0u);

    // There should be no events left for us to read
    count = read_all(true);
    ASSERT_EQ(count, 0u);

    this->DropConsumer(kTopicPath, kConsumerName);
}

UTEST_F(YdbTopicFixture, TopicReadSessionGetEvents) {
    AddConsumer(kTopicPath, kConsumerName);
    auto session = CreateReadSession(kTopicPath, kConsumerName);

    std::vector<NYdb::NTopic::TReadSessionEvent::TDataReceivedEvent> data_received_events;
    std::vector<NYdb::NTopic::TReadSessionEvent::TStartPartitionSessionEvent> start_partition_session_events;

    const auto get_and_handle_events = [&] {
        std::vector<NYdb::NTopic::TReadSessionEvent::TEvent> events;
        auto task = engine::AsyncNoTracing([&events, &session] { UASSERT_NO_THROW(events = session.GetEvents()); });
        task.WaitFor(utest::kMaxTestWaitTime);
        ASSERT_TRUE(task.IsFinished());

        ASSERT_FALSE(events.empty());

        for (auto& event : events) {
            std::visit(
                utils::Overloaded{
                    [&data_received_events](NYdb::NTopic::TReadSessionEvent::TDataReceivedEvent& e) {
                        data_received_events.push_back(std::move(e));
                    },
                    [&start_partition_session_events](NYdb::NTopic::TReadSessionEvent::TStartPartitionSessionEvent& e) {
                        e.Confirm();
                        start_partition_session_events.push_back(std::move(e));
                    },
                    [](NYdb::NTopic::TReadSessionEvent::TStopPartitionSessionEvent& e) { e.Confirm(); },
                    []([[maybe_unused]] auto& e) {
                        // do nothing
                    }
                },
                event
            );
        }
    };
    get_and_handle_events();

    ASSERT_TRUE(data_received_events.empty());
    ASSERT_FALSE(start_partition_session_events.empty());

    GetTableClient().ExecuteDataQuery(fmt::format(
        R"-(
      INSERT INTO {} (key, value)
      VALUES
        (123, "qwe"),
        (321, "xyz");
    )-",
        kTable
    ));

    get_and_handle_events();

    ASSERT_FALSE(data_received_events.empty());

    session.Close(std::chrono::milliseconds{1000});
    DropConsumer(kTopicPath, kConsumerName);
}

UTEST_F(YdbTopicFixture, AlterTopic) {
    constexpr std::string_view consumer_name = "another_test_consumer";

    NYdb::NTopic::TAlterTopicSettings settings;
    settings.AppendAddConsumers({settings, ydb::impl::ToString(consumer_name)});
    GetTopicClient().AlterTopic(kTopicPath, settings);

    auto describe_topic_result = GetNativeTopicClient().DescribeTopic(ydb::impl::ToString(kTopicPath)).GetValueSync();
    ASSERT_TRUE(describe_topic_result.IsSuccess());
    const auto& topic_description = describe_topic_result.GetTopicDescription();
    const auto& consumers = topic_description.GetConsumers();
    ASSERT_EQ(1, consumers.size());
    ASSERT_EQ(consumer_name, consumers[0].GetConsumerName());

    DropConsumer(kTopicPath, consumer_name);
}

UTEST_F(YdbTopicFixture, DescribeTopic) {
    AddConsumer(kTopicPath, kConsumerName);

    auto describe_topic_result = GetTopicClient().DescribeTopic(kTopicPath);
    ASSERT_TRUE(describe_topic_result.IsSuccess());
    const auto& topic_description = describe_topic_result.GetTopicDescription();
    const auto& consumers = topic_description.GetConsumers();
    ASSERT_EQ(1, consumers.size());
    ASSERT_EQ(kConsumerName, consumers[0].GetConsumerName());

    DropConsumer(kTopicPath, kConsumerName);
}

UTEST_F(YdbTopicWriteSessionFixture, TopicWriteSessionCreateClose) {
    auto session = CreateWriteSession();
    UASSERT_NO_THROW(session.Close(std::chrono::milliseconds{1000}));
}

UTEST_F(YdbTopicWriteSessionFixture, TopicWriteSessionGetNative) {
    auto session = CreateWriteSession();
    EXPECT_TRUE(session.GetNativeTopicWriteSession().GetInitSeqNo().Initialized());
    session.Close(std::chrono::milliseconds{1000});
}

UTEST_F(YdbTopicWriteSessionFixture, TopicWriteSessionWriteSingle) {
    auto session = CreateWriteSession();

    auto task = engine::AsyncNoTracing([&] { UASSERT_NO_THROW(WriteAndAck(session, "hello")); });
    task.WaitFor(utest::kMaxTestWaitTime);
    ASSERT_TRUE(task.IsFinished());

    session.Close(std::chrono::milliseconds{1000});
}

UTEST_F(YdbTopicWriteSessionFixture, TopicWriteSessionWriteMultiple) {
    auto session = CreateWriteSession();

    auto task = engine::AsyncNoTracing([&] {
        for (const std::string_view msg : {"msg-1", "msg-2", "msg-3"}) {
            UASSERT_NO_THROW(WriteAndAck(session, msg));
        }
    });
    task.WaitFor(utest::kMaxTestWaitTime);
    ASSERT_TRUE(task.IsFinished());

    session.Close(std::chrono::milliseconds{1000});
}

UTEST_F(YdbTopicWriteSessionFixture, TopicWriteSessionTryGetEventEmpty) {
    auto session = CreateWriteSession();

    auto task = engine::AsyncNoTracing([&] {
        // Drain TReadyToAcceptEvent so the session is established
        // and the event queue is empty.
        auto event = session.GetEvent();
        ASSERT_TRUE(std::holds_alternative<NYdb::NTopic::TWriteSessionEvent::TReadyToAcceptEvent>(event));

        // Queue is now drained — TryGetEvent must return nullopt immediately.
        EXPECT_FALSE(session.TryGetEvent().has_value());
    });
    task.WaitFor(utest::kMaxTestWaitTime);
    ASSERT_TRUE(task.IsFinished());

    session.Close(std::chrono::milliseconds{1000});
}

USERVER_NAMESPACE_END
