#pragma once

/// @file userver/utils/fixed_array.hpp
/// @brief @copybrief utils::FixedArray

#include <cstddef>
#include <memory>  // std::allocator
#include <type_traits>
#include <utility>

#include <userver/utils/assert.hpp>

USERVER_NAMESPACE_BEGIN

namespace utils {

namespace impl {

struct GenerateTag final {
  explicit GenerateTag() = default;
};

}  // namespace impl

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

  FixedArray(FixedArray&& other) noexcept;
  FixedArray& operator=(FixedArray&& other) noexcept;

  FixedArray(const FixedArray&) = delete;
  FixedArray& operator=(const FixedArray&) = delete;

  ~FixedArray();

  std::size_t size() const noexcept { return size_; }
  bool empty() const noexcept { return size_ == 0; }

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

  /// @cond
  template <class GeneratorFunc>
  FixedArray(impl::GenerateTag tag, std::size_t size,
             GeneratorFunc&& generator);
  /// @endcond

 private:
  T* NonEmptyData() noexcept {
    UASSERT(size_);
    return data();
  }

  const T* NonEmptyData() const noexcept {
    UASSERT(size_);
    return data();
  }

  T* storage_{nullptr};
  std::size_t size_{0};
};

/// @brief Applies @p generator to indices in the range `[0, size)`, storing the
/// results in a new utils::FixedArray. The generator is guaranteed to be
/// invoked in the first-to-last order.
/// @param size How many objects to generate
/// @param generator A functor that takes an index and returns an object for the
/// `FixedArray`
/// @returns `FixedArray` with the return objects of @p generator
template <class GeneratorFunc>
auto GenerateFixedArray(std::size_t size, GeneratorFunc&& generator);

template <class T>
template <class... Args>
FixedArray<T>::FixedArray(std::size_t size, Args&&... args) : size_(size) {
  if (size_ == 0) return;
  storage_ = std::allocator<T>{}.allocate(size_);

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
template <class GeneratorFunc>
FixedArray<T>::FixedArray(impl::GenerateTag /*tag*/, std::size_t size,
                          GeneratorFunc&& generator)
    : size_(size) {
  if (size_ == 0) return;
  storage_ = std::allocator<T>{}.allocate(size_);

  auto* our_begin = begin();
  auto* const our_end = end();
  std::size_t index = 0;

  try {
    for (; our_begin != our_end; ++our_begin) {
      new (our_begin) T(generator(index));
      ++index;
    }
  } catch (...) {
    std::destroy(begin(), our_begin);
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

template <class GeneratorFunc>
auto GenerateFixedArray(std::size_t size, GeneratorFunc&& generator) {
  using ResultType = std::remove_reference_t<
      std::invoke_result_t<GeneratorFunc&, std::size_t>>;
  return FixedArray<ResultType>(impl::GenerateTag{}, size,
                                std::forward<GeneratorFunc>(generator));
}

}  // namespace utils

USERVER_NAMESPACE_END
