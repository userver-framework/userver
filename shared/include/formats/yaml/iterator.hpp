#pragma once

/// @file formats/yaml/iterator.hpp
/// @brief @copybrief formats::yaml::Iterator

#include <iterator>

#include <formats/yaml/types.hpp>

#include <utils/fast_pimpl.hpp>

namespace formats {
namespace yaml {

/// @brief Iterator for `formats::yaml::Value`
template <typename iter_traits>
class Iterator final {
 public:
  using iterator_category = std::forward_iterator_tag;
  using difference_type = std::ptrdiff_t;
  using value_type = typename iter_traits::value_type;
  using reference = typename iter_traits::reference;
  using pointer = typename iter_traits::pointer;

  Iterator(const typename iter_traits::native_iter& iter, int index,
           const formats::yaml::Path& path);
  Iterator(const Iterator& other);
  Iterator(Iterator&& other) noexcept;
  Iterator& operator=(const Iterator& other);
  Iterator& operator=(Iterator&& other) noexcept;
  ~Iterator();

  Iterator operator++(int);
  Iterator& operator++();
  reference operator*() const;
  pointer operator->() const;

  bool operator==(const Iterator& other) const;
  bool operator!=(const Iterator& other) const;

  /// @brief Returns name of the referenced field
  /// @throws `TypeMismatchException` if iterated value is not an object
  std::string GetName() const;
  /// @brief Returns index of the referenced field
  /// @throws `TypeMismatchException` if iterated value is not an array
  uint32_t GetIndex() const;

 private:
  void UpdateValue() const;

 private:
  static constexpr std::size_t kNativeIterSize = 48;
  static constexpr std::size_t kNativeIterAlignment = alignof(void*);
  utils::FastPimpl<typename iter_traits::native_iter, kNativeIterSize,
                   kNativeIterAlignment>
      iter_pimpl_;
  formats::yaml::Path path_;
  // Temporary object replaced on every value access.
  mutable int index_;
  mutable value_type value_;
  mutable bool valid_;
};

}  // namespace yaml
}  // namespace formats
