#pragma once

/// @file userver/formats/json/iterator.hpp
/// @brief @copybrief formats::json::Iterator

#include <iterator>
#include <optional>
#include <string>

USERVER_NAMESPACE_BEGIN

namespace formats::json {

/// @brief Iterator for `formats::json::Value`
template <typename Traits>
class Iterator final {
 public:
  using iterator_category = std::forward_iterator_tag;
  using difference_type = std::ptrdiff_t;
  using value_type = typename Traits::ValueType;
  using reference = typename Traits::Reference;
  using pointer = typename Traits::Pointer;

  using ContainerType = typename Traits::ContainerType;

  Iterator(ContainerType container, int pos);

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
  size_t GetIndex() const;

 private:
  Iterator(ContainerType&& container, int type, int pos) noexcept;

  void UpdateValue() const;

 private:
  /// Container being iterated
  ContainerType container_;
  /// Internal container type
  int type_;
  /// Position inside container being iterated
  int pos_;
  // Temporary object replaced on every value access.
  mutable std::optional<value_type> current_;
};

}  // namespace formats::json

USERVER_NAMESPACE_END
