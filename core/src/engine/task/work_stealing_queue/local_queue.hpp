#pragma once

#include <array>
#include <atomic>
#include <cstddef>
#include <userver/utils/assert.hpp>

USERVER_NAMESPACE_BEGIN

namespace engine {

template <typename T, std::size_t Capacity>
class LocalQueue {
 public:
  LocalQueue() = default;

  size_t Size() const { return tail_.load() - head_.load(); }

  // Use only when queue empty
  void PushBulk(T** buffer, std::size_t buffer_size) {
    std::size_t curr_tail = tail_.load();
    UASSERT(tail_.load() - head_.load() == 0);
    UASSERT(buffer_size <= Capacity);

    for (std::size_t i = 0; i < buffer_size; ++i) {
      ring_buffer_[GetIndex(curr_tail + i)].store(buffer[i]);
    }

    tail_.store(curr_tail + buffer_size);
  }

  bool TryPush(T* element) {
    std::size_t curr_head = head_.load();
    size_t curr_tail = tail_.load();

    if (curr_tail - curr_head == Capacity) {
      return false;
    }

    ring_buffer_[GetIndex(curr_tail)].store(element);
    tail_.store(curr_tail + 1);
    return true;
  }

  T* Pop() {
    while (true) {
      std::size_t curr_head = head_.load();
      std::size_t curr_tail = tail_.load();
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

  std::size_t TryPopBulk(T** buffer, std::size_t buffer_size) {
    while (true) {
      std::size_t curr_head = head_.load();
      std::size_t curr_tail = tail_.load();
      std::size_t size = curr_tail - curr_head;
      std::size_t available = (buffer_size < size) ? buffer_size : size;
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
  std::size_t GetIndex(size_t pos) { return pos % (Capacity + 1); }

  std::array<std::atomic<T*>, Capacity + 1> ring_buffer_;
  std::atomic<std::size_t> head_{0};
  std::atomic<std::size_t> tail_{0};
};

}  // namespace engine

USERVER_NAMESPACE_END
