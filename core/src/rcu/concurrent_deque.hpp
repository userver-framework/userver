#pragma once

#include <array>
#include <atomic>
#include <climits>
#include <cstddef>
#include <type_traits>

#include <userver/utils/assert.hpp>

USERVER_NAMESPACE_BEGIN

namespace rcu::impl {

/// @brief A `std::deque`-like data structure that is able to grow concurrently
///
/// Uses an array of exponentially-growing blocks for storage. Once inserted,
/// the elements are never moved.
///
/// @tparam T the element type
/// @tparam MinBlockSize the number of elements in the first block
/// @tparam MaxBlockCount the maximum number of blocks
template <typename T, std::size_t MinBlockSize, std::size_t MaxBlockCount>
class ConcurrentDeque final {
 private:
  static_assert(std::is_nothrow_default_constructible_v<T>);
  static_assert(MinBlockSize != 0);
  static_assert(MaxBlockCount != 0);

  static std::size_t IndexOfBlock(std::size_t index) noexcept {
    return sizeof(unsigned long long) * CHAR_BIT -
           __builtin_clzll(index / MinBlockSize + 1) - 1;
  }

  static std::size_t IndexWithinBlock(std::size_t index) noexcept {
    return index - MinBlockSize * ((1 << IndexOfBlock(index)) - 1);
  }

  static constexpr std::size_t TotalElementsInBlocks(
      std::size_t blocks_count) noexcept {
    return MinBlockSize * ((1 << blocks_count) - 1);
  }

 public:
  static constexpr std::size_t kMaxSize = TotalElementsInBlocks(MaxBlockCount);

  /// Default-constructs at least `initial_size` elements initially
  explicit ConcurrentDeque(std::size_t initial_size) noexcept
      : reserved_blocks_count_(
            initial_size == 0 ? 0 : IndexOfBlock(initial_size - 1) + 1) {
    UASSERT(initial_size <= kMaxSize);

    for (auto& block : blocks_) {
      block.store(nullptr, std::memory_order_release);
    }

    if (initial_size != 0) {
      // Instead of allocating multiple blocks during future operator[] calls,
      // preallocate a single "super block" and store pointers to its parts
      // as first few block pointers.
      T* super_block = new T[TotalElementsInBlocks(reserved_blocks_count_)];
      std::size_t block_size = MinBlockSize;
      T* position_in_super_block = super_block;

      for (std::size_t i = 0; i != reserved_blocks_count_; ++i) {
        blocks_[i].store(position_in_super_block, std::memory_order_relaxed);
        position_in_super_block += block_size;
        block_size *= 2;
      }
    }
  }

  ~ConcurrentDeque() {
    if (reserved_blocks_count_ != 0) {
      for (std::size_t i = 1; i != reserved_blocks_count_; ++i) {
        blocks_[i].store(nullptr, std::memory_order_relaxed);
      }
    }

    for (auto& block : blocks_) {
      delete[] block.load(std::memory_order_acquire);
    }
  }

  /// Because of implementation details, the regular integral index must be
  /// converted to the internal representation before usage. `FastIndex`
  /// gives the user a chance to cache it, reducing the overhead.
  class FastIndex final {
   public:
    explicit FastIndex(std::size_t index) noexcept
        : index_of_block_(IndexOfBlock(index)),
          index_within_block_bytes_((IndexWithinBlock(index)) * sizeof(T)) {
      UASSERT(index < kMaxSize);
    }

   private:
    friend class ConcurrentDeque;

    std::size_t index_of_block_;
    std::size_t index_within_block_bytes_;
  };

  /// Accesses the element at the specified index. If the corresponding block is
  /// not yet created, creates it, default-initializing its elements.
  T& operator[](FastIndex index) noexcept {
    T* block = blocks_[index.index_of_block_].load(std::memory_order_acquire);

    if (__builtin_expect(block == nullptr, false)) {
      block = AllocateBlock(index.index_of_block_);
    }

    return *reinterpret_cast<T*>(reinterpret_cast<unsigned char*>(block) +
                                 index.index_within_block_bytes_);
  }

  /// Iterates over all the initialized elements. Does an atomic fence before
  /// iterating.
  template <typename Func>
  void Walk(const Func& walker) {
    static_assert(std::is_invocable_v<Func, T&>);
    std::atomic_thread_fence(std::memory_order_seq_cst);

    std::size_t block_size = MinBlockSize;
    for (auto& block_ptr : blocks_) {
      T* const block = block_ptr.load(std::memory_order_relaxed);
      if (block != nullptr) {
        for (std::size_t i = 0; i != block_size; ++i) {
          walker(block[i]);
        }
      }
      block_size *= 2;
    }
  }

 private:
  // Without __attribute__((noinline)), Clang 9 fails to inline 'operator[]',
  // which results in 2x slowdown in RCU benchmarks.
  T* AllocateBlock(std::size_t block_index) noexcept __attribute__((noinline)) {
    // try to create the block, default-initializing the elements
    T* expected = nullptr;
    T* const desired = new T[MinBlockSize * (1 << block_index)]{};

    if (blocks_[block_index].compare_exchange_strong(
            expected, desired, std::memory_order_acq_rel)) {
      // yay, we are the first to create the block
      return desired;
    } else {
      // another thread has created the block in-between the two checks
      delete[] desired;
      return expected;
    }
  }

  std::array<std::atomic<T*>, MaxBlockCount> blocks_;
  const std::size_t reserved_blocks_count_;
};

}  // namespace rcu::impl

USERVER_NAMESPACE_END
