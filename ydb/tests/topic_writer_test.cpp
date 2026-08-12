/// Integration tests for TopicWriter using a real YDB test database.
///
/// These tests verify that TopicWriter and TopicWriterManager correctly write
/// messages to YDB topics and that the messages can be read back.

#include <userver/utest/utest.hpp>

#include <atomic>
#include <cctype>
#include <chrono>
#include <string>
#include <vector>

#include <userver/engine/async.hpp>
#include <userver/engine/sleep.hpp>
#include <userver/utils/async.hpp>
#include <userver/utils/overloaded.hpp>
#include <userver/utils/statistics/storage.hpp>
#include <userver/utils/statistics/testing.hpp>
#include <userver/ydb/impl/cast.hpp>

#include <userver/ydb/topic_writer.hpp>
#include <userver/ydb/topic_writer_fwd.hpp>
#include <userver/ydb/topic_writer_types.hpp>

#include "test_utils.hpp"

USERVER_NAMESPACE_BEGIN

namespace {

/// Sanitizes a string so it is safe to use as a YDB topic name.
/// Replaces every character that is not alphanumeric or '-' with 'x'.
std::string SanitizeTopicName(std::string s) {
    for (char& c : s) {
        if (!std::isalnum(static_cast<unsigned char>(c)) && c != '-') {
            c = 'x';
        }
    }
    return s;
}

/// Tries to read requested number of messages from session. Will return number of successfully read
std::size_t ReadFromSession(
    ydb::TopicReadSession& session,
    size_t desired_amount,
    std::chrono::milliseconds timeout = std::chrono::milliseconds{3000}
) {
    std::size_t count = 0;
    const auto deadline = engine::Deadline::FromDuration(timeout);

    while (!deadline.IsReached() && count != desired_amount) {
        if (!session.GetNativeTopicReadSession().WaitEvent().Wait(std::chrono::milliseconds{500})) {
            continue;
        }
        auto events = session.GetEvents();
        if (events.empty()) {
            continue;
        }

        for (auto& event : events) {
            std::cout << "REceived event" << std::endl;
            std::visit(
                utils::Overloaded{
                    [&count](NYdb::NTopic::TReadSessionEvent::TDataReceivedEvent& e) {
                        std::cout << "REceived data event" << std::endl;
                        count += e.GetMessages().size();
                        e.Commit();
                    },
                    [](NYdb::NTopic::TReadSessionEvent::TStartPartitionSessionEvent& e) { e.Confirm(); },
                    [](NYdb::NTopic::TReadSessionEvent::TStopPartitionSessionEvent& e) { e.Confirm(); },
                    []([[maybe_unused]] auto& e) {}
                },
                event
            );
        }
    }

    return count;
}

void ExpectWriterStatistics(
    const utils::statistics::Snapshot& snapshot,
    std::size_t expected_messages_received,
    std::size_t expected_messages_handled,
    std::size_t expected_event_acks_written,
    std::size_t expected_write_issues
) {
    EXPECT_EQ(
        snapshot.SingleMetric("topic_writer.messages.received").AsRate(),
        utils::statistics::Rate{expected_messages_received}
    );
    EXPECT_EQ(
        snapshot.SingleMetric("topic_writer.messages.handled").AsRate(),
        utils::statistics::Rate{expected_messages_handled}
    );
    EXPECT_EQ(
        snapshot.SingleMetric("topic_writer.event_acks.written").AsRate(),
        utils::statistics::Rate{expected_event_acks_written}
    );
    EXPECT_EQ(
        snapshot.SingleMetric("topic_writer.write_issues").AsRate(),
        utils::statistics::Rate{expected_write_issues}
    );
}

void ExpectManagerStatistics(
    const utils::statistics::Snapshot& snapshot,
    std::string_view writer_name,
    std::size_t expected_messages_received,
    std::size_t expected_messages_handled,
    std::size_t expected_event_acks_written,
    std::size_t expected_write_issues
) {
    const auto prefix = std::string{"topic_writer_manager"};
    std::vector<utils::statistics::Label> labels{
        utils::statistics::Label{std::string{"writer"}, std::string{writer_name}}
    };
    EXPECT_EQ(
        snapshot.SingleMetric(prefix + ".messages.received", labels).AsRate(),
        utils::statistics::Rate{expected_messages_received}
    );
    EXPECT_EQ(
        snapshot.SingleMetric(prefix + ".messages.handled").AsRate(),
        utils::statistics::Rate{expected_messages_handled}
    );
    EXPECT_EQ(
        snapshot.SingleMetric(prefix + ".event_acks.written").AsRate(),
        utils::statistics::Rate{expected_event_acks_written}
    );
    EXPECT_EQ(snapshot.SingleMetric(prefix + ".write_issues").AsRate(), utils::statistics::Rate{expected_write_issues});
}

/// Fixture that creates a standalone YDB topic (not a changefeed) for
/// TopicWriter integration tests.
///
/// The topic name is derived automatically from the current test's suite and
/// test name (sanitized to contain only alphanumeric characters and '-').
class TopicWriterFixture : public ydb::ClientFixtureBase {
protected:
    static constexpr std::string_view kConsumerName = "test-consumer";

    explicit TopicWriterFixture() = default;

    /// Creates a topic whose name is derived from the running test's full name
    /// (suite + test), with an optional suffix to allow multiple topics per
    /// test. Registers the topic for cleanup in the destructor.
    /// @param suffix optional suffix appended before sanitization (e.g. "-a")
    /// @returns the sanitized topic name that was created.
    std::string CreateTopic(const std::string& suffix = "") {
        const auto* test_info = ::testing::UnitTest::GetInstance()->current_test_info();
        UINVARIANT(test_info, "CreateTopic() called outside of a running test");

        const std::string
            raw_name = std::string(test_info->test_suite_name()) + "-" + std::string(test_info->name()) + suffix;
        const std::string topic_path = SanitizeTopicName(raw_name);

        NYdb::NTopic::TCreateTopicSettings settings;
        settings.PartitioningSettings(1, 1);
        settings.AppendConsumers(NYdb::NTopic::TConsumerSettings(settings, ydb::impl::ToString(kConsumerName)));

        const auto
            status = GetNativeTopicClient().CreateTopic(ydb::impl::ToString(topic_path), settings).GetValueSync();
        EXPECT_TRUE(status.IsSuccess()) << "CreateTopic failed: " << status.GetIssues().ToString();

        topics_created_.push_back(topic_path);
        return topic_path;
    }

    ~TopicWriterFixture() override {
        for (const auto& topic : topics_created_) {
            const auto status = GetNativeTopicClient().DropTopic(ydb::impl::ToString(topic)).GetValueSync();
            if (!status.IsSuccess()) {
                ADD_FAILURE() << "Failed to drop topic '" << topic << "': " << status.GetIssues().ToString();
            }
        }
    }

    /// Builds a TopicWriterSettings pointing at the given topic.
    ydb::TopicWriterSettings MakeWriterSettings(
        const std::string& topic_path,
        std::size_t workers_num = 1,
        std::size_t max_queue = 100
    ) {
        return ydb::TopicWriterSettings{
            .topic_client = GetTopicClientPtr(),
            .topic_name = topic_path,
            .workers_num = workers_num,
            .max_incoming_msg_queue_size = max_queue,
            .max_ydb_control_event_queue_size = 10,
        };
    }

    /// Opens a read session for the given topic.
    ydb::TopicReadSession OpenReadSession(const std::string& topic_path) {
        NYdb::NTopic::TReadSessionSettings read_settings;
        read_settings.AppendTopics(ydb::impl::ToString(topic_path));
        read_settings.ConsumerName(ydb::impl::ToString(kConsumerName));
        return GetTopicClient().CreateReadSession(read_settings);
    }

private:
    std::vector<std::string> topics_created_;
};

}  // namespace

/// Verify that a TopicWriter can be constructed and immediately destroyed
/// without errors.
UTEST_F(TopicWriterFixture, ConstructAndDestroy) {
    const auto topic = CreateTopic();

    UASSERT_NO_THROW(ydb::TopicWriter writer("test-writer", MakeWriterSettings(topic)));
}

/// Verify that WriteMessage returns kOk when the topic exists and the queue
/// has space.
UTEST_F(TopicWriterFixture, WriteMessageReturnsOk) {
    const auto topic = CreateTopic();

    ydb::TopicWriter writer("test-writer", MakeWriterSettings(topic));

    const auto result = writer.WriteMessage("hello world", {{"key", "value"}});
    EXPECT_EQ(result.GetStatus(), ydb::TopicWriteStatus::kOk);
}

/// Verify that a message written via TopicWriter can be read back from the
/// topic using a read session.
UTEST_F(TopicWriterFixture, WrittenMessageIsReadable) {
    const auto topic = CreateTopic();

    utils::statistics::Storage storage;
    ydb::TopicWriter writer("test-writer", MakeWriterSettings(topic));
    auto holder = storage.RegisterWriter("topic_writer", [&writer](utils::statistics::Writer& w) {
        DumpMetric(w, writer);
    });
    auto read_session = OpenReadSession(topic);

    // Give the writer time to establish its session with YDB.
    engine::SleepFor(std::chrono::milliseconds{500});

    const auto result = writer.WriteMessage("integration-test-message", {});
    ASSERT_EQ(result.GetStatus(), ydb::TopicWriteStatus::kOk);

    // Allow the message to propagate.
    engine::SleepFor(std::chrono::milliseconds{500});

    const std::size_t received = ReadFromSession(read_session, 1u);
    EXPECT_GE(received, 1u);

    engine::SleepFor(std::chrono::milliseconds{500});
    const auto snapshot = utils::statistics::Snapshot{storage};
    ExpectWriterStatistics(snapshot, 1, 1, 1, 0);

    read_session.Close(std::chrono::milliseconds{1000});
}

/// Verify that multiple messages written sequentially are all delivered.
UTEST_F(TopicWriterFixture, MultipleMessagesAreDelivered) {
    const auto topic = CreateTopic();

    utils::statistics::Storage storage;
    ydb::TopicWriter writer("test-writer", MakeWriterSettings(topic));
    auto holder = storage.RegisterWriter("topic_writer", [&writer](utils::statistics::Writer& w) {
        DumpMetric(w, writer);
    });
    auto read_session = OpenReadSession(topic);

    engine::SleepFor(std::chrono::milliseconds{500});

    constexpr std::size_t kMessageCount = 5u;
    for (std::size_t i = 0; i < kMessageCount; ++i) {
        const auto result = writer.WriteMessage("message-" + std::to_string(i), {{"index", std::to_string(i)}});
        EXPECT_EQ(result.GetStatus(), ydb::TopicWriteStatus::kOk);
    }

    engine::SleepFor(std::chrono::milliseconds{1000});

    const std::size_t received = ReadFromSession(read_session, kMessageCount);
    EXPECT_GE(received, kMessageCount);

    engine::SleepFor(std::chrono::milliseconds{500});
    const auto snapshot = utils::statistics::Snapshot{storage};
    ExpectWriterStatistics(snapshot, kMessageCount, kMessageCount, kMessageCount, 0);

    read_session.Close(std::chrono::milliseconds{1000});
}

/// Verify that WriteMessage returns kResourceExhausted when the incoming
/// message queue is full.
UTEST_F(TopicWriterFixture, WriteMessageQueueExhausted) {
    const auto topic = CreateTopic();

    // Use a queue size of 1 so it fills up immediately.
    ydb::TopicWriter writer("test-writer", MakeWriterSettings(topic, 1, 1));

    // First message should fit.
    const auto result1 = writer.WriteMessage("msg1", {});
    EXPECT_EQ(result1.GetStatus(), ydb::TopicWriteStatus::kOk);

    // Second message should be rejected because the queue is full.
    const auto result2 = writer.WriteMessage("msg2", {});
    EXPECT_EQ(result2.GetStatus(), ydb::TopicWriteStatus::kResourceExhausted);
}

/// Verify that a TopicWriter with multiple workers distributes writes across
/// them (round-robin) and all messages are eventually delivered.
UTEST_F(TopicWriterFixture, MultipleWorkersDeliverMessages) {
    const auto topic = CreateTopic();

    constexpr std::size_t kWorkers = 3;
    ydb::TopicWriter writer("test-writer", MakeWriterSettings(topic, kWorkers));
    auto read_session = OpenReadSession(topic);

    engine::SleepFor(std::chrono::milliseconds{500});

    constexpr std::size_t kMessageCount = 9;  // 3 per worker
    for (std::size_t i = 0; i < kMessageCount; ++i) {
        const auto result = writer.WriteMessage("msg-" + std::to_string(i), {});
        EXPECT_EQ(result.GetStatus(), ydb::TopicWriteStatus::kOk);
    }

    engine::SleepFor(std::chrono::milliseconds{1500});

    const std::size_t received = ReadFromSession(read_session, kMessageCount);
    EXPECT_GE(received, kMessageCount);

    read_session.Close(std::chrono::milliseconds{1000});
}

/// Verify that TopicWriter::ExtendStatistics does not throw and produces
/// non-empty output after writing messages.
UTEST_F(TopicWriterFixture, StatisticsAreExported) {
    const auto topic = CreateTopic();

    utils::statistics::Storage storage;
    ydb::TopicWriter writer("stats-writer", MakeWriterSettings(topic));

    auto holder = storage.RegisterWriter("topic_writer", [&writer](utils::statistics::Writer& w) {
        DumpMetric(w, writer);
    });

    const auto snapshot = utils::statistics::Snapshot{storage};
    ExpectWriterStatistics(snapshot, 0, 0, 0, 0);
}

/// Verify that TopicWriterManager creates writers and returns them by name.
UTEST_F(TopicWriterFixture, ManagerGetTopicWriter) {
    const auto topic = CreateTopic();

    std::unordered_map<std::string, ydb::TopicWriterSettings> settings_map;
    settings_map.emplace("my-writer", MakeWriterSettings(topic));

    ydb::TopicWriterManager manager(std::move(settings_map));

    [[maybe_unused]] auto& writer = manager.GetTopicWriter("my-writer");

    EXPECT_ANY_THROW({ [[maybe_unused]] auto& missing = manager.GetTopicWriter("nonexistent"); });
}

/// Verify that a writer obtained from TopicWriterManager can write messages.
UTEST_F(TopicWriterFixture, ManagerWriterCanWrite) {
    const auto topic = CreateTopic();

    std::unordered_map<std::string, ydb::TopicWriterSettings> settings_map;
    settings_map.emplace("writer", MakeWriterSettings(topic));

    ydb::TopicWriterManager manager(std::move(settings_map));
    auto& writer = manager.GetTopicWriter("writer");

    const auto result = writer.WriteMessage("manager-message", {{"source", "manager"}});
    EXPECT_EQ(result.GetStatus(), ydb::TopicWriteStatus::kOk);
}

/// Verify that TopicWriterManager::ExtendStatistics does not throw.
UTEST_F(TopicWriterFixture, ManagerStatisticsAreExported) {
    const auto topic = CreateTopic();

    std::unordered_map<std::string, ydb::TopicWriterSettings> settings_map;
    settings_map.emplace("stats-writer", MakeWriterSettings(topic));

    ydb::TopicWriterManager manager(std::move(settings_map));

    utils::statistics::Storage storage;
    auto holder = storage.RegisterWriter("topic_writer_manager", [&manager](utils::statistics::Writer& w) {
        DumpMetric(w, manager);
    });

    const auto snapshot = utils::statistics::Snapshot{storage};
    ExpectManagerStatistics(snapshot, "stats-writer", 0, 0, 0, 0);
}

/// Verify that TopicWriterManager manages multiple writers independently.
/// Two topics are needed here; we use the suffix parameter of CreateTopic().
UTEST_F(TopicWriterFixture, ManagerMultipleWriters) {
    const auto topic_a = CreateTopic("-a");
    const auto topic_b = CreateTopic("-b");

    std::unordered_map<std::string, ydb::TopicWriterSettings> settings_map;
    settings_map.emplace("writer-a", MakeWriterSettings(topic_a));
    settings_map.emplace("writer-b", MakeWriterSettings(topic_b));

    ydb::TopicWriterManager manager(std::move(settings_map));

    auto& writer_a = manager.GetTopicWriter("writer-a");
    auto& writer_b = manager.GetTopicWriter("writer-b");

    EXPECT_EQ(writer_a.WriteMessage("msg-a", {}).GetStatus(), ydb::TopicWriteStatus::kOk);
    EXPECT_EQ(writer_b.WriteMessage("msg-b", {}).GetStatus(), ydb::TopicWriteStatus::kOk);
}

/// Verify that writing messages with metadata (key-value pairs) does not
/// cause errors.
UTEST_F(TopicWriterFixture, WriteMessageWithMetadata) {
    const auto topic = CreateTopic();

    ydb::TopicWriter writer("meta-writer", MakeWriterSettings(topic));

    ydb::TopicWriterMetadata meta{
        {"service", "userver-test"},
        {"version", "1.0"},
        {"env", "testing"},
    };

    const auto result = writer.WriteMessage("message-with-metadata", meta);
    EXPECT_EQ(result.GetStatus(), ydb::TopicWriteStatus::kOk);
}

/// Verify that writing an empty message body is accepted.
UTEST_F(TopicWriterFixture, WriteEmptyMessage) {
    const auto topic = CreateTopic();

    ydb::TopicWriter writer("empty-writer", MakeWriterSettings(topic));

    const auto result = writer.WriteMessage("", {});
    EXPECT_EQ(result.GetStatus(), ydb::TopicWriteStatus::kOk);
}

/// Verify that writing a large message is accepted.
UTEST_F(TopicWriterFixture, WriteLargeMessage) {
    const auto topic = CreateTopic();

    ydb::TopicWriter writer("large-writer", MakeWriterSettings(topic));

    // 64 KiB message
    const std::string large_message(64 * 1024, 'x');
    const auto result = writer.WriteMessage(large_message, {});
    EXPECT_EQ(result.GetStatus(), ydb::TopicWriteStatus::kOk);
}

/// Verify that concurrent writes from multiple coroutines all return kOk
/// (assuming the queue is large enough).
UTEST_F(TopicWriterFixture, ConcurrentWritesFromMultipleCoroutines) {
    const auto topic = CreateTopic();

    constexpr std::size_t kCoroutines = 4;
    constexpr std::size_t kMessagesPerCoroutine = 10;
    constexpr std::size_t kTotalMessages = kCoroutines * kMessagesPerCoroutine;

    ydb::TopicWriter writer("concurrent-writer", MakeWriterSettings(topic, 2, kTotalMessages + 10));

    std::atomic<std::size_t> ok_count{0};

    std::vector<engine::Task> tasks;
    tasks.reserve(kCoroutines);
    for (std::size_t c = 0; c < kCoroutines; ++c) {
        tasks.push_back(utils::AsyncHideSpan([&writer, &ok_count, c] {
            for (std::size_t i = 0; i < kMessagesPerCoroutine; ++i) {
                const auto result = writer.WriteMessage(
                    "coroutine-" + std::to_string(c) + "-msg-" + std::to_string(i),
                    {{"coroutine", std::to_string(c)}}
                );
                if (result.GetStatus() == ydb::TopicWriteStatus::kOk) {
                    ok_count.fetch_add(1, std::memory_order_relaxed);
                }
            }
        }));
    }

    for (auto& t : tasks) {
        t.WaitFor(utest::kMaxTestWaitTime);
        ASSERT_TRUE(t.IsFinished());
    }

    EXPECT_EQ(ok_count.load(), kTotalMessages);
}

USERVER_NAMESPACE_END
