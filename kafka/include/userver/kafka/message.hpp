#pragma once

#include <chrono>
#include <optional>

#include <userver/kafka/headers.hpp>
#include <userver/utils/fast_pimpl.hpp>
#include <userver/utils/span.hpp>
#include <userver/utils/zstring_view.hpp>

USERVER_NAMESPACE_BEGIN

namespace kafka {

namespace impl {

class ConsumerImpl;
class MessageHolder;

}  // namespace impl

/// @brief Wrapper for polled message data access.
class Message final {
public:
    ~Message();

    Message(Message&&) noexcept;
    Message& operator=(Message&&) noexcept = delete;

    Message(const Message&) = delete;
    Message& operator=(const Message&) = delete;

    const std::string& GetTopic() const;
    std::string_view GetKey() const;
    std::string_view GetPayload() const;
    std::optional<std::chrono::milliseconds> GetTimestamp() const;
    int GetPartition() const;
    std::int64_t GetOffset() const;

    /// @note Headers are parsed only when accessed first time.
    ///
    /// If name `name` passed, only headers with such name returns
    ///
    /// Usage example:
    ///
    /// - All headers:
    /// @code
    /// for (auto header : message.GetHeaders()) { /* use ...header... */}
    /// @endcode
    /// - Start own headers
    /// @code
    /// auto reader = message.GetHeaders();
    /// auto headers = std::vector<kafka::OwningHeader>{reader.begin(), reader.end()};
    /// @endcode
    HeadersReader GetHeaders() const&;
    HeadersReader GetHeaders() && = delete;

    /// @brief Returns **last** header with given name.
    /// @warning This operation has O(N) complexity, where N == number of all message headers.
    std::optional<std::string_view> GetHeader(utils::zstring_view name) const;

private:
    friend class impl::ConsumerImpl;

    explicit Message(impl::MessageHolder&& message);

    struct MessageData;
    using DataStorage = utils::FastPimpl<MessageData, 72, 8>;

    DataStorage data_;
};

using MessageBatchView = utils::span<const Message>;

}  // namespace kafka

USERVER_NAMESPACE_END
