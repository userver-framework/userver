#pragma once

/// @file userver/ydb/topic.hpp
/// @brief YDB Topic client

#include <atomic>
#include <chrono>
#include <cstdint>
#include <memory>
#include <optional>
#include <string>
#include <string_view>

#include <ydb-cpp-sdk/client/topic/client.h>
#include <ydb-cpp-sdk/client/topic/producer.h>
#include <ydb-cpp-sdk/client/types/executor/executor.h>

#include <userver/compiler/impl/lifetime.hpp>
#include <userver/engine/deadline.hpp>

USERVER_NAMESPACE_BEGIN

namespace ydb {

namespace impl {
class Driver;
struct TopicSettings;
}  // namespace impl

/// @brief Read session used to connect to one or more topics for reading
///
/// @see https://ydb.tech/docs/en/reference/ydb-sdk/topic#reading
///
/// ## Example usage:
///
/// @ref samples/ydb_service/components/topic_reader.hpp
/// @ref samples/ydb_service/components/topic_reader.cpp
///
/// @example samples/ydb_service/components/topic_reader.hpp
/// @example samples/ydb_service/components/topic_reader.cpp
class TopicReadSession final {
public:
    /// @cond
    // For internal use only.
    explicit TopicReadSession(std::shared_ptr<NYdb::NTopic::IReadSession> read_session);
    /// @endcond

    /// @brief Get read session events
    ///
    /// Waits until event occurs
    /// @param max_events_count maximum events count in batch
    /// @param max_size_bytes total size limit for data messages in batch
    /// if not specified, read session chooses event batch size automatically
    std::vector<NYdb::NTopic::TReadSessionEvent::TEvent> GetEvents(
        std::optional<std::size_t> max_events_count = {},
        size_t max_size_bytes = std::numeric_limits<size_t>::max()
    );

    /// @brief Get read session events
    ///
    /// Waits until event occurs
    /// @param settings ydb native read session settings
    std::vector<NYdb::NTopic::TReadSessionEvent::TEvent> GetEvents(
        const NYdb::NTopic::TReadSessionGetEventSettings& settings
    );

    /// @brief Close read session
    ///
    /// Waits for all commit acknowledgments to arrive.
    /// Force close after timeout
    bool Close(std::chrono::milliseconds timeout);

    /// @brief Get native read session
    ///
    /// @warning Use with care! Facilities from @ref userver/drivers/subscribable_futures.hpp can help
    /// with non-blocking wait operations.
    NYdb::NTopic::IReadSession& GetNativeTopicReadSession() USERVER_IMPL_LIFETIME_BOUND;

private:
    std::shared_ptr<NYdb::NTopic::IReadSession> read_session_;
};

/// @brief Write session used to connect to a topic for writting
///
/// @see https://ydb.tech/docs/en/reference/ydb-sdk/topic#write
class TopicWriteSession final {
public:
    /// @cond
    /// For internal use only.
    explicit TopicWriteSession(std::shared_ptr<NYdb::NTopic::IWriteSession> write_session);
    /// @endcond

    /// @brief Wait for the next write session event
    ///
    /// Suspends the current coroutine until an event is available, then returns it without blocking the thread.
    NYdb::NTopic::TWriteSessionEvent::TEvent GetEvent();

    /// @brief Poll for a write session event without waiting
    ///
    /// Returns the next buffered event immediately if one is available, or `std::nullopt` if the event queue is empty.
    /// Does not suspend the coroutine.
    ///
    /// @note Sometimes may return `std::nullopt` even if an event is available. Intended for use in loops.
    std::optional<NYdb::NTopic::TWriteSessionEvent::TEvent> TryGetEvent();

    /// @brief Write a messsage using a continuation token from TReadyToAcceptEvent
    ///
    /// Must be called only after receiving TReadyToAcceptEvent from GetEvent() or TryGetEvent().
    void Write(NYdb::NTopic::TContinuationToken&& token, NYdb::NTopic::TWriteMessage&& message);

    /// @brief Close write session
    ///
    /// Waits for all in-flights messages to be acknowledged.
    /// Force closes after timeout
    bool Close(std::chrono::milliseconds timeout);

    /// @brief Get native write session
    ///
    /// @warning Use with care! Facilities from @ref userver/drivers/subscribable_futures.hpp can help
    /// with non-blocking wait operations.
    NYdb::NTopic::IWriteSession& GetNativeTopicWriteSession() USERVER_IMPL_LIFETIME_BOUND;

private:
    std::shared_ptr<NYdb::NTopic::IWriteSession> write_session_;
};

/// @brief Simple write session used to write messages to a topic without
/// manually handling write session events.
///
/// This is a userver-native analogue of YDB SDK
/// `ISimpleBlockingWriteSession`: methods may wait for YDB flow-control
/// continuation tokens, but waiting suspends the current coroutine instead of
/// blocking an OS thread. It wraps a single `IWriteSession`; its simple API does
/// not expose the native event loop or acknowledgments.
///
/// @see https://ydb.tech/docs/en/reference/ydb-sdk/topic#write
class TopicSimpleWriteSession final {
public:
    /// @cond
    /// For internal use only.
    explicit TopicSimpleWriteSession(std::shared_ptr<NYdb::NTopic::IWriteSession> write_session);
    /// @endcond

    TopicSimpleWriteSession(const TopicSimpleWriteSession&) = delete;
    TopicSimpleWriteSession& operator=(const TopicSimpleWriteSession&) = delete;
    TopicSimpleWriteSession(TopicSimpleWriteSession&&) noexcept;
    TopicSimpleWriteSession& operator=(TopicSimpleWriteSession&&) noexcept;

    /// @brief Write a single message.
    ///
    /// Waits until YDB provides a continuation token or until `deadline`.
    /// @returns true if the message was enqueued for writing, false if the
    /// deadline was reached or the session was closed before a token arrived.
    bool Write(
        NYdb::NTopic::TWriteMessage&& message,
        NYdb::TTransactionBase* tx = nullptr,
        engine::Deadline deadline = {}
    );

    /// @brief Write a single message using basic message options.
    bool Write(
        std::string_view data,
        std::optional<std::uint64_t> seq_no = std::nullopt,
        std::optional<std::chrono::system_clock::time_point> create_timestamp = std::nullopt,
        engine::Deadline deadline = {}
    );

    /// @brief Wait until initial SeqNo is discovered from the server.
    std::uint64_t GetInitSeqNo(engine::Deadline deadline = {});

    /// @brief Close the write session.
    ///
    /// Waits for all in-flight messages to be acknowledged.
    /// Force closes after `timeout`.
    /// @returns `true` if all writes were completed and acknowledged. Returns
    /// `false` if the timeout expired and some writes were aborted; in that
    /// case their delivery is not guaranteed and should be handled as an
    /// application-level delivery failure.
    bool Close(std::chrono::milliseconds timeout);

    /// @brief Returns true if the write session is alive and active.
    bool IsAlive() const noexcept;

    /// @brief Get native write session
    ///
    /// @warning Use with care! Facilities from @ref userver/drivers/subscribable_futures.hpp can help
    /// with non-blocking wait operations.
    NYdb::NTopic::IWriteSession& GetNativeTopicWriteSession() USERVER_IMPL_LIFETIME_BOUND;

private:
    std::optional<NYdb::NTopic::TContinuationToken> WaitForToken(engine::Deadline deadline);

    std::shared_ptr<NYdb::NTopic::IWriteSession> write_session_;
    std::atomic_bool closed_{false};
};

/// @brief Settings for TopicProducer.
class TopicProducerSettings final : public NYdb::NTopic::TProducerSettings {
    using NYdb::NTopic::TProducerSettings::MaxBlockTimeout;
    using NYdb::NTopic::TProducerSettings::MaxBlockTimeout_;
    using NYdb::NTopic::TProducerSettings::MaxMemoryUsage;
    using NYdb::NTopic::TProducerSettings::MaxMemoryUsage_;
};

/// @brief Native YDB producer used for partition-aware writes to a topic.
///
/// Unlike TopicSimpleWriteSession, this is a wrapper around the YDB SDK
/// `IProducer`. It selects partitions by message key or explicit partition and
/// manages the corresponding write sessions internally.
///
/// Write() fails immediately if the internal buffer is overloaded.
///
/// @see https://ydb.tech/docs/en/reference/ydb-sdk/topic#write
class TopicProducer final {
public:
    /// @cond
    /// For internal use only.
    explicit TopicProducer(std::shared_ptr<NYdb::NTopic::IProducer> producer);
    /// @endcond

    /// @brief Write a single message to the topic.
    ///
    /// Adds the message to the internal buffer and returns its queueing status.
    /// Fails immediately with `EWriteStatus::Timeout` if the buffer is full.
    /// Use Flush() to wait for the buffered messages to be persistently written.
    NYdb::NTopic::TWriteResult Write(NYdb::NTopic::TWriteMessage&& message);

    /// @brief Flush all buffered messages to the server.
    ///
    /// Waits until all in-flight messages are acknowledged.
    /// @param deadline timeout for flush completion
    NYdb::NTopic::TFlushResult Flush(engine::Deadline deadline = {});

    /// @brief Close the producer.
    ///
    /// Waits for all in-flight messages to be acknowledged.
    /// Force closes after timeout.
    NYdb::NTopic::TCloseResult Close(std::chrono::milliseconds timeout);

    /// @brief Get native producer.
    ///
    /// @warning Use with care! Facilities from @ref userver/drivers/subscribable_futures.hpp can help
    /// with non-blocking wait operations.
    NYdb::NTopic::IProducer& GetNativeTopicProducer() USERVER_IMPL_LIFETIME_BOUND;

private:
    std::shared_ptr<NYdb::NTopic::IProducer> producer_;
};

/// @ingroup userver_clients
///
/// @brief YDB Topic Client
///
/// @see https://ydb.tech/docs/en/concepts/topic
class TopicClient final {
public:
    static constexpr std::uint64_t kDefaultProducerMaxMemoryUsageBytes = 2ULL * 1024 * 1024 * 1024;

    /// @cond
    // For internal use only.
    TopicClient(std::shared_ptr<impl::Driver> driver, impl::TopicSettings settings);
    /// @endcond

    ~TopicClient();

    /// Alter topic
    void AlterTopic(const std::string& path, const NYdb::NTopic::TAlterTopicSettings& settings);

    /// Describe topic
    NYdb::NTopic::TDescribeTopicResult DescribeTopic(const std::string& path);

    /// Create read session
    TopicReadSession CreateReadSession(const NYdb::NTopic::TReadSessionSettings& settings);

    /// Create write session
    TopicWriteSession CreateWriteSession(const NYdb::NTopic::TWriteSessionSettings& settings);

    /// Create simple write session.
    ///
    /// @note Event handlers from `settings` are not compatible with
    /// TopicSimpleWriteSession. They are reset with a warning.
    TopicSimpleWriteSession CreateSimpleWriteSession(const NYdb::NTopic::TWriteSessionSettings& settings);

    /// Create producer.
    ///
    /// Unlike TopicSimpleWriteSession, TopicProducer is a native YDB
    /// multi-session producer: it routes each message by key or explicit
    /// partition and fails immediately if its buffer is overloaded. Flush()
    /// waits for persistence without closing the producer;
    /// TopicSimpleWriteSession instead flushes pending writes as part of
    /// Close().
    /// @param max_memory_usage_bytes maximum buffered message memory in bytes
    TopicProducer CreateProducer(
        const TopicProducerSettings& settings,
        std::uint64_t max_memory_usage_bytes = kDefaultProducerMaxMemoryUsageBytes
    );

    /// Get native topic client
    /// @warning Use with care! Facilities from
    /// `<core/include/userver/drivers/subscribable_futures.hpp>` can help with
    /// non-blocking wait operations.
    NYdb::NTopic::TTopicClient& GetNativeTopicClient() USERVER_IMPL_LIFETIME_BOUND;

private:
    std::shared_ptr<impl::Driver> driver_;
    // Owned executors: Stop() only after `topic_client_` is destroyed (see
    // ~TopicClient). Joining these threads after the native client is gone
    // avoids atexit use-after-destroy (e.g. SEGV in TCodecMap). Stopping them
    // while TTopicClient is still alive would deadlock or stall writes.
    NYdb::IExecutor::TPtr compression_executor_;
    NYdb::IExecutor::TPtr handlers_executor_;
    // `reset()` in ~TopicClient runs before Stop() on the executors above.
    std::optional<NYdb::NTopic::TTopicClient> topic_client_;
};

}  // namespace ydb

USERVER_NAMESPACE_END
