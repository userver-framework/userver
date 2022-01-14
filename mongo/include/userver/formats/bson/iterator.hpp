#pragma once

/// @file userver/formats/bson/iterator.hpp
/// @brief @copybrief formats::bson::Iterator

#include <iterator>
#include <optional>
#include <string>
#include <type_traits>
#include <unordered_map>
#include <variant>
#include <vector>

#include <userver/formats/bson/types.hpp>
#include <userver/formats/common/iterator_direction.hpp>

USERVER_NAMESPACE_BEGIN

namespace formats::bson {

/// Iterator for BSON values
template <typename ValueType, common::IteratorDirection Direction =
                                  common::IteratorDirection::kForward>
class Iterator final {
 public:
  using iterator_category = std::forward_iterator_tag;
  using difference_type = ptrdiff_t;
  using value_type = std::remove_const_t<ValueType>;
  using reference = ValueType&;
  using pointer = ValueType*;

  /// @cond
  using NativeIter = std::variant<impl::ParsedArray::const_iterator,
                                  impl::ParsedArray::const_reverse_iterator,
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
  template <typename T = void>
  std::string GetName() const {
    static_assert(Direction == common::IteratorDirection::kForward,
                  "Reverse iterator should be used only on arrays or null, "
                  "they do not have GetName()");
    return GetNameImpl();
  }

  /// @brief Returns index of currently selected array element
  /// @throws TypeMismatchException if iterated value is not an array
  uint32_t GetIndex() const;

 private:
  std::string GetNameImpl() const;
  void UpdateValue() const;

  impl::ValueImpl* iterable_;
  NativeIter it_;
  mutable std::optional<value_type> current_;
};

}  // namespace formats::bson

USERVER_NAMESPACE_END
