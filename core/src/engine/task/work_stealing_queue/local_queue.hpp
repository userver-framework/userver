#pragma once

#include <array>
#include <atomic>
#include <cstddef>
#include <span>

#include <userver/utils/assert.hpp>

USERVER_NAMESPACE_BEGIN

namespace engine::impl {
class TaskContext;
}  // namespace engine::impl

namespace engine::fast {

inline constexpr std::size_t kLocalQueueCapacity = 255;
inline constexpr std::size_t kLocalQueueBufferSize = kLocalQueueCapacity + 1;

static_assert((kLocalQueueBufferSize & (kLocalQueueBufferSize - 1)) == 0, "buffer size must be a power of 2");

class SPMCQueue final {
public:
    std::size_t GetSizeUpperBound() const noexcept {
        const std::size_t head = head_.load(std::memory_order_relaxed);
        const std::size_t tail = tail_.load(std::memory_order_relaxed);
        return tail - head;
    }

    void PushMany(std::span<impl::TaskContext* const> buffer) noexcept {
        const std::size_t head = head_.load(std::memory_order_acquire);
        const std::size_t tail = tail_.load(std::memory_order_relaxed);
        UASSERT(tail - head + buffer.size() <= kLocalQueueCapacity);

        for (std::size_t i = 0; i < buffer.size(); ++i) {
            buffer_[ToIndex(tail + i)].item.store(buffer[i], std::memory_order_relaxed);
        }
        tail_.store(tail + buffer.size(), std::memory_order_release);
    }

    bool TryPush(impl::TaskContext* item) noexcept {
        const std::size_t head = head_.load(std::memory_order_acquire);
        const std::size_t tail = tail_.load(std::memory_order_relaxed);

        if (IsFull(head, tail)) {
            return false;
        }

        UASSERT(item != nullptr);
        buffer_[ToIndex(tail)].item.store(item, std::memory_order_relaxed);
        tail_.store(tail + 1, std::memory_order_release);
        return true;
    }

    impl::TaskContext* TryPop() noexcept {
        while (true) {
            std::size_t head = head_.load(std::memory_order_acquire);
            const std::size_t tail = tail_.load(std::memory_order_relaxed);

            UASSERT(head <= tail);

            if (IsEmpty(head, tail)) {
                return nullptr;
            }

            impl::TaskContext* const item = buffer_[ToIndex(head)].item.load(std::memory_order_relaxed);
            if (head_.compare_exchange_weak(head, head + 1, std::memory_order_release, std::memory_order_relaxed)) {
                return item;
            }
        }
    }

    std::size_t Grab(std::span<impl::TaskContext*> out_buffer) noexcept {
        while (true) {
            std::size_t head = head_.load(std::memory_order_acquire);
            const std::size_t tail = tail_.load(std::memory_order_acquire);

            UASSERT(head <= tail);

            const std::size_t size = tail - head;
            if (size == 0) {
                return 0;
            }

            const std::size_t to_grab = std::min(out_buffer.size(), size);

            for (std::size_t i = 0; i < to_grab; ++i) {
                out_buffer[i] = buffer_[ToIndex(head + i)].item.load(std::memory_order_relaxed);
            }

            if (head_.compare_exchange_weak(head, head + to_grab, std::memory_order_release, std::memory_order_relaxed))
            {
                return to_grab;
            }
        }
    }

private:
    static std::size_t ToIndex(std::size_t pos) noexcept { return pos & (kLocalQueueBufferSize - 1); }

    static bool IsEmpty(std::size_t head, std::size_t tail) noexcept { return head == tail; }

    static bool IsFull(std::size_t head, std::size_t tail) noexcept { return tail - head + 1 >= kLocalQueueBufferSize; }

    struct Slot final {
        std::atomic<impl::TaskContext*> item{nullptr};
    };

    std::array<Slot, kLocalQueueBufferSize> buffer_{};
    std::atomic<std::size_t> head_{0};
    std::atomic<std::size_t> tail_{0};
};

}  // namespace engine::fast

USERVER_NAMESPACE_END
