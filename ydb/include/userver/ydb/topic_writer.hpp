#pragma once

/// @file userver/ydb/topic_writer.hpp
/// @brief YDB Topic writer interface and implementation

#include <memory>
#include <optional>
#include <string>
#include <unordered_map>
#include <vector>

#include <userver/compiler/impl/lifetime.hpp>
#include <userver/utils/impl/transparent_hash.hpp>
#include <userver/utils/statistics/fwd.hpp>

#include <userver/ydb/topic.hpp>
#include <userver/ydb/topic_writer_types.hpp>

USERVER_NAMESPACE_BEGIN

// Forward declaration
namespace ydb::impl {
class TopicWriterWorker;
}

namespace ydb {

/// @brief Settings for TopicWriter
struct TopicWriterSettings {
    /// Maximum number of workers allowed per topic writer
    static constexpr std::size_t kMaxWorkers = 100;

    /// @brief Level of the statistics provided to monitoring
    enum class StatisticLevel {
        /// Consolidated stats for whole topic writer (sum of stats for all workers
        /// within this writer)
        kWriter,
        /// Stats for all workers individually
        kWorker
    };

    /// Topic client for communication with YDB. Note that TopicClient
    /// is a client to the topic service, not a client to a specific topic.
    std::shared_ptr<ydb::TopicClient> topic_client;
    /// Topic name to write messages into
    std::string topic_name;
    /// Number of workers used to write messages into the topic. Messages are
    /// distributed across workers in round-robin fashion. Increasing this
    /// value improves throughput but does not preserve message ordering.
    std::size_t workers_num{1};
    /// Maximum number of messages that can be buffered in the incoming queue
    /// before WriteMessage starts returning kResourceExhausted
    std::size_t max_incoming_msg_queue_size{100};
    /// Maximum number of control events from YDB that can be buffered
    /// before the worker stops reading new events from the session
    std::size_t max_ydb_control_event_queue_size{10};

    /// Desired level for collected statistics
    StatisticLevel stats_level{StatisticLevel::kWriter};
};

/// @brief Base interface for writing messages to a YDB topic
class TopicWriterBase {
public:
    /// @brief Write a message to YDB
    ///
    /// @param[in] message message data to be sent to the topic
    /// @param[in] meta key-value metadata to attach to the message
    ///
    /// @return result of the message handling
    [[nodiscard]] virtual TopicWriteResult WriteMessage(std::string message, const TopicWriterMetadata& meta) = 0;

    TopicWriterBase() = default;
    TopicWriterBase(const TopicWriterBase&) = delete;
    TopicWriterBase(TopicWriterBase&&) = delete;
    TopicWriterBase& operator=(const TopicWriterBase&) = delete;
    TopicWriterBase& operator=(TopicWriterBase&&) = delete;
    virtual ~TopicWriterBase() = default;
};

/// @brief Takes messages provided by the user and sends them to a YDB topic
///
/// This class can be created directly, or you can use TopicWriterManager. If
/// created via TopicWriterManager, it will take care of collecting and writing
/// statistics for you. Otherwise, behaviour is unchanged.
class TopicWriter final : public TopicWriterBase {
public:
    /// @brief Creates a new topic writer
    ///
    /// @param[in] name identifier used for statistics labeling; should be
    /// unique across the service to avoid metric collisions
    /// @param[in] settings controls which topic to write into and tuning
    /// parameters such as worker count and queue sizes
    TopicWriter(std::string name, TopicWriterSettings settings);

    /// @brief Writes a message to YDB
    ///
    /// Internally a worker is selected for the message sending in round-robin
    /// fashion across all configured workers.
    [[nodiscard]] TopicWriteResult WriteMessage(std::string message, const TopicWriterMetadata& meta) override;

private:
    friend void DumpMetric(utils::statistics::Writer& writer, const TopicWriter& topic_writer);

    // Alias for topic. This is configuration name from component
    // config and it will also be used for statistics.
    const std::string name_;

    const TopicWriterSettings settings_;
    std::shared_ptr<ydb::TopicClient> topic_client_;
    // for round robin algorithm
    std::atomic<std::size_t> current_worker_{0};
    std::vector<std::shared_ptr<impl::TopicWriterWorker>> workers_;
};

/// @brief Manages a collection of named YDB topic writers
///
/// Provides access to individual TopicWriter instances by name,
/// and aggregates their statistics.
class TopicWriterManager final {
public:
    /// @brief Constructs a TopicWriterManager with the given per-topic settings
    /// @param settings map from topic writer name to its settings; the keys
    /// become the names used to look up writers via GetTopicWriter
    explicit TopicWriterManager(std::unordered_map<std::string, TopicWriterSettings> settings);

    /// @brief Returns the topic writer for the provided name
    /// @param name key that was used in the settings map passed to the
    /// constructor; determines which topic writer is returned
    /// @return reference to the particular topic writer, or throws an exception
    /// no topic writer with the specified name
    TopicWriter& GetTopicWriter(std::string_view name) USERVER_IMPL_LIFETIME_BOUND;

private:
    friend void DumpMetric(utils::statistics::Writer& writer, const TopicWriterManager& manager);

    utils::impl::TransparentMap<std::string, std::shared_ptr<TopicWriter>> topic_writers_;
};

/// @brief Dumps aggregated metrics for all topic writers into the statistics writer
/// @param writer statistics writer to dump into
/// @param manager the topic writer manager whose metrics to dump
void DumpMetric(utils::statistics::Writer& writer, const TopicWriterManager& manager);

/// @brief Dumps metrics for a TopicWriter into the statistics writer
/// @param writer statistics writer to dump into
/// @param topic_writer the topic writer whose metrics to dump
void DumpMetric(utils::statistics::Writer& writer, const TopicWriter& topic_writer);

}  // namespace ydb

USERVER_NAMESPACE_END
