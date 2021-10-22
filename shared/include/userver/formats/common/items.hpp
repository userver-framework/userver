#pragma once

#include <cstddef>
#include <iterator>
#include <string>
#include <type_traits>

USERVER_NAMESPACE_BEGIN

namespace formats::common {

template <typename Value>
class ItemsWrapper final {
 public:
  class Iterator {
   public:
    using RawIterator = decltype(std::declval<Value>().begin());

    struct ItValue {
      std::string key;
      typename RawIterator::reference value;
    };

    using iterator_category = std::forward_iterator_tag;
    using difference_type = std::ptrdiff_t;
    using value_type = ItValue;
    using reference = ItValue;

    explicit Iterator(RawIterator it) : it_(it) {}
    Iterator(const Iterator& other) = default;
    Iterator(Iterator&& other) noexcept = default;

    Iterator& operator=(const Iterator& other) = default;
    Iterator& operator=(Iterator&& other) noexcept = default;

    ItValue operator*() const { return {it_.GetName(), *it_}; }

    Iterator operator++(int) {
      auto it = *this;
      return {it++};
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

  ItemsWrapper(Value&& value) : value_(static_cast<Value&&>(value)) {}

  auto begin() const { return cbegin(); }
  auto end() const { return cend(); }
  auto cbegin() const { return Iterator(value_.begin()); }
  auto cend() const { return Iterator(value_.end()); }

 private:
  Value value_;
};

template <typename Value>
inline auto Items(Value&& value) {
  // when passed an lvalue, store by reference
  // when passed an rvalue, store by value
  return ItemsWrapper<Value>(static_cast<Value&&>(value));
}

}  // namespace formats::common

USERVER_NAMESPACE_END
