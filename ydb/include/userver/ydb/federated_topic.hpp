#pragma once

/// @file userver/ydb/federated_topic.hpp
/// @brief YDB Federated Topic client
///
/// Federated Topic SDK serves as a wrapper over Topic SDK, coordinates
/// reading from federated YDB installations topics, has a subset of functions
/// of usual Topic SDK
///
/// @ref userver/ydb/topic.hpp

#include <chrono>
#include <memory>

#include <ydb-cpp-sdk/client/federated_topic/federated_topic.h>

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
/// @ref samples/ydb_service/components/federated_topic_reader.hpp
/// @ref samples/ydb_service/components/federated_topic_reader.cpp
///
/// @example samples/ydb_service/components/federated_topic_reader.hpp
/// @example samples/ydb_service/components/federated_topic_reader.cpp
class FederatedTopicReadSession final {
public:
    /// @cond
    // For internal use only.
    explicit FederatedTopicReadSession(std::shared_ptr<NYdb::NFederatedTopic::IFederatedReadSession> read_session);
    /// @endcond

    /// @brief Get read session events
    ///
    /// Waits until event occurs
    /// @param max_events_count maximum events count in batch
    /// @param max_size_bytes total size limit for data messages in batch
    /// if not specified, read session chooses event batch size automatically
    std::vector<NYdb::NFederatedTopic::TReadSessionEvent::TEvent> GetEvents(
        std::optional<std::size_t> max_events_count = {},
        size_t max_size_bytes = std::numeric_limits<size_t>::max()
    );

    /// @brief Close read session
    ///
    /// Waits for all commit acknowledgments to arrive.
    /// Force close after timeout
    bool Close(std::chrono::milliseconds timeout);

    /// Get native read session
    /// @warning Use with care! Facilities from
    /// `<core/include/userver/drivers/subscribable_futures.hpp>` can help with
    /// non-blocking wait operations.
    std::shared_ptr<NYdb::NFederatedTopic::IFederatedReadSession> GetNativeTopicReadSession();

private:
    std::shared_ptr<NYdb::NFederatedTopic::IFederatedReadSession> read_session_;
};

/// @ingroup userver_clients
///
/// @brief YDB Federated Topic Client
///
/// @see https://ydb.tech/docs/en/concepts/topic
class FederatedTopicClient final {
public:
    /// @cond
    // For internal use only.
    FederatedTopicClient(std::shared_ptr<impl::Driver> driver, impl::TopicSettings settings);
    /// @endcond

    ~FederatedTopicClient();

    /// Create read session
    FederatedTopicReadSession CreateReadSession(const NYdb::NFederatedTopic::TFederatedReadSessionSettings& settings);

    /// Get native topic client
    /// @warning Use with care! Facilities from
    /// `<core/include/userver/drivers/subscribable_futures.hpp>` can help with
    /// non-blocking wait operations.
    NYdb::NFederatedTopic::TFederatedTopicClient& GetNativeTopicClient();

private:
    std::shared_ptr<impl::Driver> driver_;
    NYdb::NFederatedTopic::TFederatedTopicClient topic_client_;
};

}  // namespace ydb

USERVER_NAMESPACE_END
