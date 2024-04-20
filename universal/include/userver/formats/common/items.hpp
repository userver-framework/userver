#pragma once

/// @file userver/formats/common/items.hpp
/// @brief @copybrief formats::common::Items()
/// @ingroup userver_universal

#include <cstddef>
#include <iterator>
#include <string>
#include <type_traits>

USERVER_NAMESPACE_BEGIN

namespace formats::common {

/// @see formats::common::ItemsWrapper
template <typename Value>
struct ItemsWrapperValue final {
  std::string key;
  typename decltype(std::declval<Value&>().begin())::reference value;
};

/// @brief Wrapper for handy python-like iteration over a map.
///
/// See formats::common::Items() for usage example
template <typename Value>
class ItemsWrapper final {
 public:
  /// Exposition-only, do not instantiate directly, use `iterator` and
  /// `const_iterator` aliases instead.
  template <bool Const>
  class Iterator final {
    using Base = std::conditional_t<Const, const Value, Value>;
    using RawIterator = decltype(std::declval<Base&>().begin());

   public:
    using iterator_category = std::forward_iterator_tag;
    using difference_type = std::ptrdiff_t;
    using value_type = ItemsWrapperValue<Base>;
    using reference = value_type;
    using pointer = void;

    /// @cond
    explicit Iterator(RawIterator it) : it_(std::move(it)) {}
    /// @endcond

    Iterator(const Iterator& other) = default;
    Iterator(Iterator&& other) noexcept = default;

    Iterator& operator=(const Iterator& other) = default;
    Iterator& operator=(Iterator&& other) noexcept = default;

    reference operator*() const { return {it_.GetName(), *it_}; }

    Iterator operator++(int) {
      auto it = *this;
      ++*this;
      return it;
    }

    Iterator& operator++() {
      ++it_;
      return *this;
    }

    bool operator==(const Iterator& other) const { return it_ == other.it_; }

    bool operator!=(const Iterator& other) const { return !(*this == other); }

   private:
    RawIterator it_;
  };

  /// @brief Satisfies `std::forward_iterator`, `std::common_iterator`,
  /// with `std::iter_reference_t` equal to
  /// formats::common::ItemsWrapperValue<Value>.
  using iterator = Iterator<false>;

  /// @brief Satisfies `std::forward_iterator`, `std::common_iterator`,
  /// with `std::iter_reference_t` equal to
  /// formats::common::ItemsWrapperValue<const Value>.
  using const_iterator = Iterator<true>;

  /// @cond
  explicit ItemsWrapper(Value&& value) : value_(static_cast<Value&&>(value)) {}
  /// @endcond

  iterator begin() { return iterator(value_.begin()); }
  iterator end() { return iterator(value_.end()); }
  const_iterator begin() const { return const_iterator(value_.begin()); }
  const_iterator end() const { return const_iterator(value_.end()); }

 private:
  Value value_;
};

/// @brief Wrapper for handy python-like iteration over a map
///
/// @code
/// for (const auto& [name, value]: Items(map)) ...
/// @endcode
///
/// To move out values:
/// @code
/// for (auto [name, value]: Items(map)) {
///   vector.push_back(std::move(name));
///   // value is a const reference and can not be moved
/// }
/// @endcode
template <typename Value>
ItemsWrapper<Value> Items(Value&& value) {
  // when passed an lvalue, store by reference
  // when passed an rvalue, store by value
  return ItemsWrapper<Value>(static_cast<Value&&>(value));
}

}  // namespace formats::common

USERVER_NAMESPACE_END
