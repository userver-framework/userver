#pragma once

/// @file userver/ydb/topic_writer_types.hpp
/// @brief Types for YDB topic writer

#include <memory>
#include <string>
#include <string_view>
#include <unordered_map>

#include <ydb-cpp-sdk/client/topic/write_events.h>

#include <userver/utils/impl/internal_tag.hpp>

USERVER_NAMESPACE_BEGIN

namespace ydb {

namespace impl {
struct TopicWriterHandlingResult;
}

/// Metadata associated with a YDB topic message (key-value pairs)
using TopicWriterMetadata = std::unordered_map<std::string, std::string>;

/// YDB write acknowledgement event state.
/// @see https://ydb.tech/docs/en/reference/ydb-sdk/topic#write
using TopicWriteEventState = NYdb::NTopic::TWriteSessionEvent::TWriteAck::EEventState;

/// @brief Provides status code for storing messages to the internal queue
enum class TopicWriteStatus : std::size_t {
    /// Message has been put into the internal queue
    kOk,
    /// Internal queue is full
    kResourceExhausted,
    /// Other error
    kFail,
    /// Not used for the moment, we accept all messages for handling
    kNotReady,
    /// @warning Do not use this value
    kMaxCount
};

/// @brief Provides the result of the message handling
///
/// @note Although initially this interface was intended to be awaitable,
///       underlying YDB driver doesn't support waiting for result without
///       enabled deduplication.
class TopicWriteResult {
public:
    /// @brief Constructs a write result
    /// @param status the status of the write operation
    explicit TopicWriteResult(TopicWriteStatus status);

    /// @cond
    // For internal use only.
    /// @brief Constructs a write result
    /// @param status the status of the write operation
    /// @param context internal handling result context (may be nullptr)
    TopicWriteResult(
        utils::impl::InternalTag,
        TopicWriteStatus status,
        std::shared_ptr<impl::TopicWriterHandlingResult> context
    );
    /// @endcond

    /// Status of whether the message has been accepted for handling by the
    /// library. Note that this status does not reflect the result of the
    /// message processing by YDB itself.
    TopicWriteStatus GetStatus() const { return status_; }

private:
    TopicWriteStatus status_;
    std::shared_ptr<impl::TopicWriterHandlingResult> context_;
};

/// @brief Converts TopicWriteStatus to a human-readable string view
/// @param status the status to convert
/// @return string_view representation of the status
std::string_view ToStringView(TopicWriteStatus status);

}  // namespace ydb

USERVER_NAMESPACE_END
