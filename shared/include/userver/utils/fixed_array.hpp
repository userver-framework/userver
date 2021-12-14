#pragma once

/// @file userver/utils/fixed_array.hpp
/// @brief @copybrief utils::FixedArray

#include <memory>   // std::allocator
#include <utility>  // std::swap

#include <userver/utils/assert.hpp>

USERVER_NAMESPACE_BEGIN

namespace utils {

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
  /// Make an array and initialize each element with "args"
  template <class... Args>
  explicit FixedArray(std::size_t size, Args&&... args);

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

  T& front() noexcept { return *data(); }
  const T& front() const noexcept { return *data(); }

  T& back() noexcept { return *(data() + size_ - 1); }
  const T& back() const noexcept { return *(data() + size_ - 1); }

  T* data() noexcept { return storage_; }
  const T* data() const noexcept { return storage_; }

  T* begin() noexcept { return data(); }
  T* end() noexcept { return data() + size_; }
  const T* begin() const noexcept { return data(); }
  const T* end() const noexcept { return data() + size_; }
  const T* cbegin() const noexcept { return data(); }
  const T* cend() const noexcept { return data() + size_; }

 private:
  T* storage_;
  std::size_t size_;
};

template <class T>
template <class... Args>
FixedArray<T>::FixedArray(std::size_t size, Args&&... args)
    : storage_(std::allocator<T>{}.allocate(size)), size_(size) {
  UASSERT(size > 0);
  if (!size) {
    return;
  }

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
