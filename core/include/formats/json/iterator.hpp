#pragma once

#include <iterator>

#include <formats/json/types.hpp>

namespace formats {
namespace json {

/// @brief Iterator for `formats::json::Value`
template <typename iter_traits>
class Iterator {
 public:
  using iterator_category = std::forward_iterator_tag;
  using difference_type = typename iter_traits::native_iter::difference_type;
  using value_type = typename iter_traits::value_type;
  using reference = typename iter_traits::reference;
  using pointer = typename iter_traits::pointer;

  Iterator(const NativeValuePtr& root,
           const typename iter_traits::native_iter& iter,
           const formats::json::Path& path);
  Iterator(const Iterator& other);
  Iterator(Iterator&& other) noexcept;
  Iterator& operator=(const Iterator& other);
  Iterator& operator=(Iterator&& other) noexcept;

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
  NativeValuePtr root_;
  typename iter_traits::native_iter iter_;
  formats::json::Path path_;
  // Temporary object replaced on every value access.
  mutable value_type value_;
  mutable bool valid_;
};

}  // namespace json
}  // namespace formats
