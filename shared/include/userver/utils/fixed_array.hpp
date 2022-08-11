#pragma once

/// @file userver/utils/fixed_array.hpp
/// @brief @copybrief utils::FixedArray

#include <iterator>  // std::size
#include <memory>    // std::allocator
#include <type_traits>
#include <utility>

#include <userver/utils/assert.hpp>

USERVER_NAMESPACE_BEGIN

namespace utils {

struct FromRangeTag {};

/// @ingroup userver_containers
///
/// @brief A fixed-size array with the size determined at runtime.
///
/// The array also allows initializing each of the array elements with the same
/// parameters:
/// @snippet src/utils/fixed_array_test.cpp  Sample FixedArray
template <class T>
class FixedArray final {
 public:
  using iterator = T*;
  using const_iterator = const T*;

  /// Make an empty array
  FixedArray() = default;

  /// Make an array and initialize each element with "args"
  template <class... Args>
  explicit FixedArray(std::size_t size, Args&&... args);

  /// Make an array and copy-initialize elements from @p range
  template <class Range>
  FixedArray(FromRangeTag tag, Range&& range);

  FixedArray(FixedArray&& other) noexcept;
  FixedArray& operator=(FixedArray&& other) noexcept;

  FixedArray(const FixedArray&) = delete;
  FixedArray& operator=(const FixedArray&) = delete;

  ~FixedArray();

  std::size_t size() const noexcept { return size_; }

  const T& operator[](std::size_t i) const noexcept {
    UASSERT(i < size_);
    return data()[i];
  }

  T& operator[](std::size_t i) noexcept {
    UASSERT(i < size_);
    return data()[i];
  }

  T& front() noexcept { return *NonEmptyData(); }
  const T& front() const noexcept { return *NonEmptyData(); }

  T& back() noexcept { return *(NonEmptyData() + size_ - 1); }
  const T& back() const noexcept { return *(NonEmptyData() + size_ - 1); }

  T* data() noexcept { return storage_; }
  const T* data() const noexcept { return storage_; }

  T* begin() noexcept { return data(); }
  T* end() noexcept { return data() + size_; }
  const T* begin() const noexcept { return data(); }
  const T* end() const noexcept { return data() + size_; }
  const T* cbegin() const noexcept { return data(); }
  const T* cend() const noexcept { return data() + size_; }

 private:
  T* NonEmptyData() noexcept {
    UASSERT(size_);
    return data();
  }

  const T* NonEmptyData() const noexcept {
    UASSERT(size_);
    return data();
  }

  T* storage_;
  std::size_t size_;
};

template <class Range>
FixedArray(FromRangeTag tag, Range&& range)
    ->FixedArray<std::decay_t<decltype(*range.begin())>>;

template <class T>
template <class... Args>
FixedArray<T>::FixedArray(std::size_t size, Args&&... args)
    : storage_(std::allocator<T>{}.allocate(size)), size_(size) {
  if (size == 0) return;

  auto* begin = data();
  try {
    for (auto* end = begin + size - 1; begin != end; ++begin) {
      new (begin) T(args...);
    }
    new (begin) T(std::forward<Args>(args)...);
  } catch (...) {
    std::destroy(data(), begin);
    std::allocator<T>{}.deallocate(storage_, size);
    throw;
  }
}

template <class T>
template <class Range>
FixedArray<T>::FixedArray(FromRangeTag /*tag*/, Range&& range)
    : storage_(nullptr), size_(std::size(std::as_const(range))) {
  storage_ = std::allocator<T>{}.allocate(size_);

  auto* begin = data();
  auto* const end = begin + size_;
  auto their_begin = range.begin();

  try {
    for (; begin != end; ++begin) {
      new (begin) T(*their_begin);
      ++their_begin;
    }
  } catch (...) {
    std::destroy(data(), begin);
    std::allocator<T>{}.deallocate(storage_, size_);
    throw;
  }
}

template <class T>
FixedArray<T>::FixedArray(FixedArray&& other) noexcept
    : storage_(std::exchange(other.storage_, nullptr)),
      size_(std::exchange(other.size_, 0)) {}

template <class T>
FixedArray<T>& FixedArray<T>::operator=(FixedArray&& other) noexcept {
  std::swap(storage_, other.storage_);
  std::swap(size_, other.size_);
  return *this;
}

template <class T>
FixedArray<T>::~FixedArray() {
  std::destroy(begin(), end());
  std::allocator<T>{}.deallocate(storage_, size_);
}

}  // namespace utils

USERVER_NAMESPACE_END
