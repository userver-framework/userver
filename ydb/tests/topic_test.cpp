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
    read_session_settings.EventHandlers_.SimpleDataHandlers(
        [handler = std::move(data_handler)](const auto&) { handler(); }, commit_data_events
    );
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
    read_session_settings.FederatedEventHandlers_.SimpleDataHandlers(
        [handler = std::move(data_handler)](const auto&) { handler(); }, commit_data_events
    );

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

    auto ReadAll = [&](bool commit_data_event) {
        std::atomic<size_t> data_events_count = 0;
        TypeParam session = this->CreateReadSessionWithDataHandler(
            kTopicPath, kConsumerName, [&data_events_count]() { data_events_count++; }, commit_data_event
        );

        auto task = engine::AsyncNoSpan([&session] {
            UASSERT_NO_THROW(session.GetNativeTopicReadSession()->WaitEvent().Wait(std::chrono::milliseconds{1000}));
        });
        task.WaitFor(std::chrono::milliseconds{1000});
        Y_ENSURE(task.IsFinished());

        session.Close(std::chrono::milliseconds{1000});
        return data_events_count.load();
    };

    // Read entire topic without commiting events, leaving consumer's offset unchanged
    size_t count = ReadAll(false);
    ASSERT_TRUE(count > 0u);

    // Read entire topic once again and commit each event, changing consumer's offset
    count = ReadAll(true);
    ASSERT_TRUE(count > 0u);

    // There should be no events left for us to read
    count = ReadAll(true);
    ASSERT_EQ(count, 0u);

    this->DropConsumer(kTopicPath, kConsumerName);
}

UTEST_F(YdbTopicFixture, TopicReadSessionGetEvents) {
    AddConsumer(kTopicPath, kConsumerName);
    auto session = CreateReadSession(kTopicPath, kConsumerName);

    std::vector<NYdb::NTopic::TReadSessionEvent::TDataReceivedEvent> data_received_events;
    std::vector<NYdb::NTopic::TReadSessionEvent::TStartPartitionSessionEvent> start_partition_session_events;

    const auto GetAndHandleEvents = [&] {
        std::vector<NYdb::NTopic::TReadSessionEvent::TEvent> events;
        auto task = engine::AsyncNoSpan([&events, &session] { UASSERT_NO_THROW(events = session.GetEvents()); });
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
                    }},
                event
            );
        }
    };
    GetAndHandleEvents();

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

    GetAndHandleEvents();

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

USERVER_NAMESPACE_END
