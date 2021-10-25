#pragma once

/// @file userver/utils/fixed_array.hpp
/// @brief @copybrief utils::FixedArray

#include <memory>
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

  ~FixedArray() { std::destroy(begin(), end()); }

  std::size_t size() const noexcept { return size_; }

  const T& operator[](std::size_t i) const noexcept {
    UASSERT(i < size_);
    return get()[i];
  }

  T& operator[](std::size_t i) noexcept {
    UASSERT(i < size_);
    return get()[i];
  }

  T& front() noexcept { return *get(); }
  const T& front() const noexcept { return *get(); }

  T& back() noexcept { return *(get() + size_ - 1); }
  const T& back() const noexcept { return *(get() + size_ - 1); }

  T* data() noexcept { return get(); }
  const T* data() const noexcept { return get(); }

  T* begin() noexcept { return get(); }
  T* end() noexcept { return get() + size_; }
  const T* begin() const noexcept { return get(); }
  const T* end() const noexcept { return get() + size_; }
  const T* cbegin() const noexcept { return get(); }
  const T* cend() const noexcept { return get() + size_; }

 private:
  T* get() const noexcept { return reinterpret_cast<T*>(storage_.get()); }

  std::unique_ptr<unsigned char[]> storage_;
  std::size_t size_;
};

template <class T>
template <class... Args>
FixedArray<T>::FixedArray(std::size_t size, Args&&... args)
    : storage_(new unsigned char[sizeof(T) * size]), size_(size) {
  UASSERT(size > 0);
  if (!size) {
    return;
  }

  auto* begin = get();
  try {
    for (auto* end = begin + size - 1; begin != end; ++begin) {
      new (begin) T(args...);
    }
    new (begin) T(std::forward<Args>(args)...);
  } catch (...) {
    std::destroy(get(), begin);
    throw;
  }
}

}  // namespace utils

USERVER_NAMESPACE_END
