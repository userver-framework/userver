#pragma once

#include <cstddef>
#include <iterator>
#include <string>

namespace formats::common {

template <typename Value>
class ItemsWrapper final {
 public:
  class Iterator {
   public:
    struct ItValue {
      std::string key;
      const Value& value;
    };
    using iterator_category = std::forward_iterator_tag;
    using difference_type = std::ptrdiff_t;
    using value_type = ItValue;
    using reference = const Value&;
    using pointer = const Value*;

    using RawIterator = decltype(Value{}.begin());

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

  ItemsWrapper(const Value& value) : value_(value) {}

  auto begin() const { return cbegin(); }
  auto end() const { return cend(); }
  auto cbegin() const { return Iterator(value_.begin()); }
  auto cend() const { return Iterator(value_.end()); }

 private:
  const Value& value_;
};

template <typename Value>
inline auto Items(const Value& value) {
  return common::ItemsWrapper<typename std::decay<Value>::type>(value);
}

}  // namespace formats::common
