#pragma once

/// @file formats/bson/iterator.hpp
/// @brief @copybrief formats::bson::Iterator

#include <iterator>
#include <optional>
#include <string>
#include <type_traits>
#include <unordered_map>
#include <variant>
#include <vector>

#include <formats/bson/types.hpp>

namespace formats::bson {

/// Iterator for BSON values
template <typename ValueType>
class Iterator final {
 public:
  using iterator_category = std::forward_iterator_tag;
  using difference_type = ptrdiff_t;
  using value_type = std::remove_const_t<ValueType>;
  using reference = ValueType&;
  using pointer = ValueType*;

  /// @cond
  using NativeIter = std::variant<impl::ParsedArray::const_iterator,
                                  impl::ParsedDocument::const_iterator>;

  Iterator(impl::ValueImpl&, NativeIter);
  /// @endcond

  Iterator(const Iterator&);
  Iterator(Iterator&&) noexcept;
  Iterator& operator=(const Iterator&);
  Iterator& operator=(Iterator&&) noexcept;

  /// @name Forward iterator requirements
  /// @{
  Iterator operator++(int);
  Iterator& operator++();
  reference operator*() const;
  pointer operator->() const;

  bool operator==(const Iterator&) const;
  bool operator!=(const Iterator&) const;
  /// @}

  /// @brief Returns name of currently selected document field
  /// @throws TypeMismatchException if iterated value is not a document
  std::string GetName() const;

  /// @brief Returns index of currently selected array element
  /// @throws TypeMismatchException if iterated value is not an array
  uint32_t GetIndex() const;

 private:
  void UpdateValue() const;

  impl::ValueImpl* iterable_;
  NativeIter it_;
  mutable std::optional<value_type> current_;
};

}  // namespace formats::bson
