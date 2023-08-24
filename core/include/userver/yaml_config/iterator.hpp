#pragma once

#include <iterator>

#include <userver/formats/yaml/value.hpp>

USERVER_NAMESPACE_BEGIN

namespace yaml_config {

template <typename iter_traits>
class Iterator final {
 public:
  using YamlIterator = formats::yaml::Value::const_iterator;
  using iterator_category = std::forward_iterator_tag;
  using difference_type = std::ptrdiff_t;
  using value_type = typename iter_traits::value_type;
  using reference = typename iter_traits::reference;
  using pointer = typename iter_traits::pointer;

  Iterator(const value_type& container, YamlIterator it)
      : container_(&container), it_(std::move(it)) {}

  Iterator(const Iterator& other);
  Iterator(Iterator&& other) noexcept;
  Iterator& operator=(const Iterator&);
  Iterator& operator=(Iterator&&) noexcept;

  Iterator operator++(int) {
    current_.reset();
    return Iterator{*container_, it_++};
  }
  Iterator& operator++() {
    ++it_;
    current_.reset();
    return *this;
  }
  reference operator*() const {
    UpdateValue();
    return *current_;
  }
  pointer operator->() const {
    UpdateValue();
    return &(*current_);
  }

  /// Return whether this is iterator over object or over array
  /// @returns formats::common::kArray or formats::common::kObject
  formats::common::Type GetIteratorType() const {
    return it_.GetIteratorType();
  }

  // Get member name - only if iterator is over object
  auto GetName() const { return it_.GetName(); }

  bool operator==(const Iterator& other) const { return it_ == other.it_; }
  bool operator!=(const Iterator& other) const { return it_ != other.it_; }

 private:
  void UpdateValue() const;

  // Pointer to the 'container' yaml - because substitution parsing
  // only works with container[index/key] statements
  const value_type* container_{nullptr};
  // Iterator over container. We actually only use its GetIndex/GetName
  // members
  YamlIterator it_;
  mutable std::optional<value_type> current_;
};

}  // namespace yaml_config

USERVER_NAMESPACE_END
