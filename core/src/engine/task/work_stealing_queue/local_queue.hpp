#pragma once

#include <array>
#include <atomic>
#include <cstddef>

#include <userver/utils/assert.hpp>
#include <userver/utils/span.hpp>

USERVER_NAMESPACE_BEGIN

namespace engine {

template <typename T, std::size_t Capacity>
class LocalQueue {
 public:
  LocalQueue() = default;

  std::size_t GetSize() const noexcept {
    return static_cast<std::size_t>(
        std::max(static_cast<std::ptrdiff_t>(tail_.load() - head_.load()),
                 std::ptrdiff_t{0}));
  }

  // For producer only
  std::size_t FreeSpace() const noexcept { return Capacity - GetSize(); }

  // Returns the number of elements added
  // Adds elements from the beginning of `buffer`
  // TODO TryPush?
  size_t PushBulk(utils::span<T*> buffer) {
    const std::size_t curr_tail = tail_.load();
    const size_t curr_head = head_.load();
    const size_t to_push_count = std::min(
        buffer.size(),
        Capacity - static_cast<std::size_t>(std::max(
                       static_cast<std::ptrdiff_t>(curr_tail - curr_head),
                       std::ptrdiff_t{0})));

    for (std::size_t i = 0; i < to_push_count; ++i) {
      ring_buffer_[GetIndex(curr_tail + i)].store(buffer[i]);
    }

    tail_.store(curr_tail + to_push_count);
    return to_push_count;
  }

  bool TryPush(T* element) {
    const std::size_t curr_head = head_.load();
    const std::size_t curr_tail = tail_.load();

    if (curr_tail - curr_head == Capacity) {
      return false;
    }

    ring_buffer_[GetIndex(curr_tail)].store(element);
    tail_.store(curr_tail + 1);
    return true;
  }

  T* TryPop() {
    while (true) {
      std::size_t curr_head = head_.load();
      const std::size_t curr_tail = tail_.load();
      UASSERT(curr_head <= curr_tail);

      if (curr_head == curr_tail) {
        return nullptr;
      }

      T* element = ring_buffer_[GetIndex(curr_head)].load();
      if (head_.compare_exchange_strong(curr_head, curr_head + 1)) {
        return element;
      }
    }
  }

  std::size_t TryPopBulk(utils::span<T*> buffer) {
    while (true) {
      std::size_t curr_head = head_.load();
      const std::size_t curr_tail = tail_.load();
      const std::size_t size = curr_tail - curr_head;
      const std::size_t available =
          (buffer.size() < size) ? buffer.size() : size;
      UASSERT(curr_head <= curr_tail);

      if (available == 0) {
        return 0;
      }

      for (std::size_t i = 0; i < available; ++i) {
        buffer[i] = ring_buffer_[GetIndex(curr_head + i)].load();
      }

      if (head_.compare_exchange_strong(curr_head, curr_head + available)) {
        return available;
      }
    }
  }

 private:
  std::size_t GetIndex(std::size_t pos) { return pos % (Capacity + 1); }

  std::array<std::atomic<T*>, Capacity + 1> ring_buffer_{};
  std::atomic<std::size_t> head_{0};
  std::atomic<std::size_t> tail_{0};
};

}  // namespace engine

USERVER_NAMESPACE_END
