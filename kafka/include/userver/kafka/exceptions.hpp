#pragma once

#include <cstdint>
#include <stdexcept>
#include <string_view>

USERVER_NAMESPACE_BEGIN

namespace kafka {

/// @brief Base exception thrown by Producer::Send and Producer::SendAsync
/// on send or delivery errors.
class SendException : public std::runtime_error {
public:
    using std::runtime_error::runtime_error;

    /// @brief Returns whether it makes sense to retry failed send.
    ///
    /// @see
    /// https://docs.confluent.io/platform/current/clients/librdkafka/html/md_INTRODUCTION.html#autotoc_md8
    bool IsRetryable() const noexcept;

protected:
    SendException(const char* what, bool is_retryable);

private:
    const bool is_retryable_{false};
};

class DeliveryTimeoutException final : public SendException {
    static constexpr const char* kWhat{
        "Message is not delivered after `delivery_timeout` milliseconds. Hint: "
        "Adjust `delivery_timeout` and `queue_buffering_*` options or manually "
        "retry the send request."};

public:
    DeliveryTimeoutException();
};

class QueueFullException final : public SendException {
    static constexpr const char* kWhat{
        "The sending queue is full - send request cannot be scheduled. Hint: "
        "Manually retry the error or increase `queue_buffering_max_messages` "
        "and/or `queue_buffering_max_kbytes` config option."};

public:
    QueueFullException();
};

class MessageTooLargeException final : public SendException {
    static constexpr const char* kWhat{
        "Message size exceeds configured limit. Hint: increase "
        "`message_max_bytes` config option."};

public:
    MessageTooLargeException();
};

class UnknownTopicException final : public SendException {
    static constexpr const char* kWhat{"Given topic does not exist in cluster."};

public:
    UnknownTopicException();
};

class UnknownPartitionException final : public SendException {
    static constexpr const char* kWhat = "Topic does not have given partition.";

public:
    UnknownPartitionException();
};

/// @brief Exception thrown when there is an error retrieving the offset range.
class OffsetRangeException : public std::runtime_error {
public:
    using std::runtime_error::runtime_error;

public:
    OffsetRangeException(std::string_view what, std::string_view topic, std::uint32_t partition);
};

class OffsetRangeTimeoutException final : public OffsetRangeException {
    static constexpr const char* kWhat = "Timeout while fetching offsets.";

public:
    OffsetRangeTimeoutException(std::string_view topic, std::uint32_t partition);
};

class TopicNotFoundException final : public std::runtime_error {
public:
    using std::runtime_error::runtime_error;
};

/// @brief Exception thrown when fetching metadata.
class GetMetadataException : public std::runtime_error {
public:
    using std::runtime_error::runtime_error;

public:
    GetMetadataException(std::string_view what, std::string_view topic);
};

class GetMetadataTimeoutException final : public GetMetadataException {
    static constexpr const char* kWhat = "Timeout while getting metadata.";

public:
    GetMetadataTimeoutException(std::string_view topic);
};

/// @brief Exception thrown when parsing consumed messages headers.
/// @ref Message::GetHeaders
class ParseHeadersException final : std::runtime_error {
    static constexpr const char* kWhat = "Failed to parse headers";

public:
    ParseHeadersException(std::string_view error);
};

/// @brief Exception thrown when Seek* process failed.
/// @ref ConsumeScope::Seek
/// @ref ConsumeScope::SeekToBeginning
/// @ref ConsumeScope::SeekToEnd
class SeekException final : public std::runtime_error {
public:
    using std::runtime_error::runtime_error;
};

/// @brief Exception thrown when Seek* arguments are invalid.
/// @ref ConsumeScope::Seek
/// @ref ConsumeScope::SeekToBeginning
/// @ref ConsumeScope::SeekToEnd
class SeekInvalidArgumentException final : public std::invalid_argument {
public:
    using std::invalid_argument::invalid_argument;
};

}  // namespace kafka

USERVER_NAMESPACE_END
