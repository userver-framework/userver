#pragma once

/// @file formats/bson/iterator.hpp
/// @brief @copybrief formats::bson::Iterator

#include <iterator>
#include <string>
#include <unordered_map>
#include <vector>

#include <boost/variant.hpp>

#include <formats/bson/types.hpp>

namespace formats {
namespace bson {

/// Iterator for BSON values
template <typename ValueType>
class Iterator {
 public:
  using iterator_category = std::forward_iterator_tag;
  using difference_type = ptrdiff_t;
  using value_type = ValueType;
  using reference = ValueType&;
  using pointer = ValueType*;

  /// @cond
  using NativeIter = boost::variant<impl::ParsedArray::const_iterator,
                                    impl::ParsedDocument::const_iterator>;

  class ArrowProxy {
   public:
    explicit ArrowProxy(ValueType);
    ValueType* operator->() { return &value_; }

   private:
    ValueType value_;
  };

  Iterator(impl::ValueImpl&, NativeIter);
  /// @endcond

  /// @name Forward iterator requirements
  /// @{
  Iterator operator++(int);
  Iterator& operator++();
  ValueType operator*() const;
  ArrowProxy operator->() const;

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
  impl::ValueImpl& iterable_;
  NativeIter it_;
};

}  // namespace bson
}  // namespace formats
