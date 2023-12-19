#pragma once

#include <array>
#include <atomic>
#include <cstddef>
#include <cstdint>
#include <optional>

#include <userver/utils/assert.hpp>
#include <userver/utils/flags.hpp>

USERVER_NAMESPACE_BEGIN

namespace engine::ev {

namespace impl {

class DoubleBufferingState final {
 public:
  using IndexType = std::size_t;

  class ProducerLock;
  class ConsumerLock;

  void Reset() noexcept;

 private:
  IndexType LockProducer();
  void UnlockProducer(IndexType locked_index) noexcept;

  IndexType LockConsumer();
  void UnlockConsumer(IndexType locked_index) noexcept;

  static constexpr auto kNoneLocked = static_cast<IndexType>(-1);

  enum class DoubleBufferFlags : std::uint32_t {
    kNone = 0,

    kLocked0 = 1 << 0,
    kLocked1 = 1 << 1,

    kDataIn0 = 1 << 2,
    kDataIn1 = 1 << 3,
  };

  utils::AtomicFlags<DoubleBufferFlags> state_{};
};

class DoubleBufferingState::ProducerLock {
 public:
  explicit ProducerLock(DoubleBufferingState& state)
      : state_(state), locked_index_(state.LockProducer()) {}
  ~ProducerLock() { state_.UnlockProducer(locked_index_); }

  ProducerLock(const ProducerLock&) = delete;
  ProducerLock& operator=(const ProducerLock&) = delete;
  ProducerLock(ProducerLock&&) = delete;
  ProducerLock& operator=(ProducerLock&&) = delete;

  IndexType GetLockedIndex() const noexcept { return locked_index_; }

 private:
  DoubleBufferingState& state_;
  IndexType locked_index_;
};

class DoubleBufferingState::ConsumerLock {
 public:
  explicit ConsumerLock(DoubleBufferingState& state)
      : state_(state), locked_index_(state.LockConsumer()) {}
  ~ConsumerLock() { state_.UnlockConsumer(locked_index_); }

  ConsumerLock(const ConsumerLock&) = delete;
  ConsumerLock& operator=(const ConsumerLock&) = delete;
  ConsumerLock(ConsumerLock&&) = delete;
  ConsumerLock& operator=(ConsumerLock&&) = delete;

  IndexType GetLockedIndex() const noexcept {
    UASSERT_MSG(*this, "Check for locked data first");
    return locked_index_;
  }

  explicit operator bool() const noexcept {
    return locked_index_ != kNoneLocked;
  }

 private:
  DoubleBufferingState& state_;
  IndexType locked_index_;
};

}  // namespace impl

/// @ingroup userver_concurrency
///
/// @brief A limited single-producer, single-consumer channel.
///
/// If the producer provides the data too fast, only the new version of data
/// is kept.
///
/// If the consumer finds that the producer is writing at the moment, it
/// bails immediately.
///
/// @note Unused versions of data may not be discarded automatically, you
/// might need to reset the pipe when the last transfer is known to be done.
template <typename Data>
class DataPipeToEv final {
 public:
  void Push(Data&& data) {
    static_assert(std::is_nothrow_move_assignable_v<Data>);

    impl::DoubleBufferingState::ProducerLock producer{state_};
    double_buffer_[producer.GetLockedIndex()] = std::move(data);
  }

  std::optional<Data> TryPop() {
    std::optional<Data> ret;

    impl::DoubleBufferingState::ConsumerLock consumer{state_};
    if (consumer) {
      ret = std::move(double_buffer_[consumer.GetLockedIndex()]);
    }

    return ret;
  }

  void UnsafeReset() {
    double_buffer_.fill({});
    state_.Reset();
  }

 private:
  std::array<Data, 2> double_buffer_{};

  impl::DoubleBufferingState state_;
};

}  // namespace engine::ev

USERVER_NAMESPACE_END
