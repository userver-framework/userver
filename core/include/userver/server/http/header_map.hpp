#pragma once

#include <iterator>
#include <optional>
#include <string>
#include <vector>

#include <userver/utils/checked_pointer.hpp>
#include <userver/utils/fast_pimpl.hpp>

USERVER_NAMESPACE_BEGIN

namespace server::http {

namespace header_map_impl {
class Map;
}

class HeaderMap final {
 public:
  class Iterator final {
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

    struct UnderlyingIterator;
    using UnderlyingIteratorImpl = utils::FastPimpl<UnderlyingIterator, 8, 8>;

    Iterator();
    Iterator(UnderlyingIteratorImpl it);
    ~Iterator();

    Iterator(const Iterator& other);
    Iterator(Iterator&& other) noexcept;
    Iterator& operator=(const Iterator& other);
    Iterator& operator=(Iterator&& other) noexcept;

    Iterator operator++(int);
    Iterator& operator++();

    reference operator*();
    const_reference operator*() const;
    pointer operator->();
    const_pointer operator->() const;

    bool operator==(const Iterator& other) const;
    bool operator!=(const Iterator& other) const;

   private:
    void UpdateCurrentValue() const;

    UnderlyingIteratorImpl it_;
    mutable std::optional<EntryProxy> current_value_;
  };
  using iterator = Iterator;
  using const_iterator = Iterator;

  HeaderMap();
  ~HeaderMap();
  HeaderMap(const HeaderMap& other);
  HeaderMap(HeaderMap&& other) noexcept;

  std::size_t size() const noexcept;
  bool empty() const noexcept;
  void clear();

  Iterator find(const std::string& key) const;

  void Insert(std::string key, std::string value);
  // TODO : think about `string_view value` here
  void InsertOrAppend(std::string key, std::string value);

  void Erase(const std::string& key);

  Iterator begin();
  Iterator begin() const;
  Iterator end();
  Iterator end() const;

 private:
  utils::FastPimpl<header_map_impl::Map, 80, 8> impl_;
};

}  // namespace server::http

USERVER_NAMESPACE_END
