#pragma once

#include <chrono>
#include <optional>

#include <userver/utils/fast_pimpl.hpp>
#include <userver/utils/span.hpp>

USERVER_NAMESPACE_BEGIN

namespace kafka {

namespace impl {

class ConsumerImpl;

}  // namespace impl

/// @brief RAII wrapper for polled message data.
/// @note All `Message` instances must be destroyed before `Consumer` stop
class Message final {
  struct Data;
  using DataStorage = utils::FastPimpl<Data, 16 + 32 + 16, 8>;

 public:
  ~Message();

  Message(Message&&) = default;

  const std::string& GetTopic() const;
  std::string_view GetKey() const;
  std::string_view GetPayload() const;
  std::optional<std::chrono::milliseconds> GetTimestamp() const;
  int GetPartition() const;
  std::int64_t GetOffset() const;

 private:
  friend class impl::ConsumerImpl;

  explicit Message(DataStorage data);

  DataStorage data_;
};

using MessageBatchView = utils::span<const Message>;

}  // namespace kafka

USERVER_NAMESPACE_END
