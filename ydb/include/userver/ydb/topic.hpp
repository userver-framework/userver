#pragma once

/// @file userver/ydb/topic.hpp
/// @brief YDB Topic client

#include <chrono>
#include <memory>
#include <optional>
#include <string>

#include <ydb-cpp-sdk/client/topic/client.h>
#include <ydb-cpp-sdk/client/types/executor/executor.h>

#include <userver/compiler/impl/lifetime.hpp>

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

/// @ingroup userver_clients
///
/// @brief YDB Topic Client
///
/// @see https://ydb.tech/docs/en/concepts/topic
class TopicClient final {
public:
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
