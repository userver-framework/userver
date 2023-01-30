#pragma once

#include <iterator>
#include <optional>
#include <string>
#include <vector>

USERVER_NAMESPACE_BEGIN

namespace server::http {

namespace header_map_impl {
struct Entry;
}

class HeaderMapIterator final {
 public:
  struct EntryProxy final {
    const std::string& first;
    std::string& second;
  };

  using iterator_category = std::forward_iterator_tag;
  using difference_type = std::ptrdiff_t;
  using value_type = EntryProxy;
  using reference = EntryProxy&;
  using const_reference = const EntryProxy&;
  using pointer = EntryProxy*;
  using const_pointer = const EntryProxy*;

  using UnderlyingIterator = std::vector<header_map_impl::Entry>::iterator;

  HeaderMapIterator();
  explicit HeaderMapIterator(UnderlyingIterator it);
  ~HeaderMapIterator();

  HeaderMapIterator(const HeaderMapIterator& other);
  HeaderMapIterator(HeaderMapIterator&& other) noexcept;
  HeaderMapIterator& operator=(const HeaderMapIterator& other);
  HeaderMapIterator& operator=(HeaderMapIterator&& other) noexcept;

  HeaderMapIterator operator++(int);
  HeaderMapIterator& operator++();

  reference operator*();
  const_reference operator*() const;
  pointer operator->();
  const_pointer operator->() const;

  bool operator==(const HeaderMapIterator& other) const;
  bool operator!=(const HeaderMapIterator& other) const;

 private:
  void UpdateCurrentValue() const;

  UnderlyingIterator it_;
  mutable std::optional<EntryProxy> current_value_;
};

class HeaderMapConstIterator final {
 public:
  struct ConstEntryProxy final {
    const std::string& first;
    const std::string& second;
  };

  using iterator_category = std::forward_iterator_tag;
  using difference_type = std::ptrdiff_t;
  using value_type = ConstEntryProxy;
  using reference = ConstEntryProxy&;
  using const_reference = const ConstEntryProxy&;
  using pointer = ConstEntryProxy*;
  using const_pointer = const ConstEntryProxy*;

  using UnderlyingIterator = std::vector<header_map_impl::Entry>::const_iterator;

  HeaderMapConstIterator();
  explicit HeaderMapConstIterator(UnderlyingIterator it);
  ~HeaderMapConstIterator();

  HeaderMapConstIterator(const HeaderMapConstIterator& other);
  HeaderMapConstIterator(HeaderMapConstIterator&& other) noexcept;
  HeaderMapConstIterator& operator=(const HeaderMapConstIterator& other);
  HeaderMapConstIterator& operator=(HeaderMapConstIterator&& other) noexcept;

  HeaderMapConstIterator operator++(int);
  HeaderMapConstIterator& operator++();

  reference operator*();
  const_reference operator*() const;
  pointer operator->();
  const_pointer operator->() const;

  bool operator==(const HeaderMapConstIterator& other) const;
  bool operator!=(const HeaderMapConstIterator& other) const;

 private:
  void UpdateCurrentValue() const;

  UnderlyingIterator it_;
  mutable std::optional<ConstEntryProxy> current_value_;
};

}  // namespace server::http

USERVER_NAMESPACE_END
