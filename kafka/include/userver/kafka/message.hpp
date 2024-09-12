#pragma once

#include <chrono>
#include <optional>

#include <userver/utils/fast_pimpl.hpp>
#include <userver/utils/span.hpp>

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

  Message(Message&&) = default;
  Message& operator=(Message&&) noexcept = delete;

  Message(const Message&) = delete;
  Message& operator=(const Message&) = delete;

  const std::string& GetTopic() const;
  std::string_view GetKey() const;
  std::string_view GetPayload() const;
  std::optional<std::chrono::milliseconds> GetTimestamp() const;
  int GetPartition() const;
  std::int64_t GetOffset() const;

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
