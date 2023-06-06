#pragma once

#include <initializer_list>
#include <iterator>
#include <string>
#include <vector>

#include <userver/formats/json_fwd.hpp>
#include <userver/formats/parse/to.hpp>
#include <userver/http/predefined_header.hpp>
#include <userver/utils/fast_pimpl.hpp>

USERVER_NAMESPACE_BEGIN

namespace http::headers {

class TestsHelper;

namespace header_map {
class Map;
}

/// @brief Container that maps case-insensitive header name into header value.
///
/// Allows storing up to 24576 name->value pairs, after that an attempt to
/// insert a new pair will throw.
///
/// Has an anti-hashdos collisions resolution built-in, and the capacity limit
/// might be lowered in case of an attack being detected.
///
/// Iterators/pointers invalidation loosely matches that of std::vector:
/// * if an insertion took place, iterators/pointers are invalidated;
/// * successful erase invalidates the iterator being erased and `begin` (it's
/// implemented via swap and pop_back idiom, and `begin` is actually an `rbegin`
/// of underlying vector).
class HeaderMap final {
 public:
  /// Iterator
  class Iterator;
  /// Const iterator
  class ConstIterator;

  using iterator = Iterator;
  using const_iterator = ConstIterator;

  using key_type = std::string;
  using mapped_type = std::string;

  /// The exception being thrown in case of capacity overflow
  class TooManyHeadersException final : public std::runtime_error {
    using std::runtime_error::runtime_error;
  };

  /// Default constructor.
  HeaderMap();
  /// Constructor from initializer list: `HeaderMap({{"a", "b"}, {"c", "d"}})`.
  /// Its unspecified which pair is inserted in case of names not being unique.
  HeaderMap(std::initializer_list<std::pair<std::string, std::string>> headers);
  /// Constructor from initializer list: `HeaderMap({{"a", "b"}, {"c", "d"}})`.
  /// Its unspecified which pair is inserted in case of names not being unique.
  HeaderMap(
      std::initializer_list<std::pair<PredefinedHeader, std::string>> headers);
  /// Constructor with capacity: preallocates `capacity` elements for internal
  /// storage.
  HeaderMap(std::size_t capacity);
  /// Constructor from iterator pair:
  /// `HeaderMap{key_value_pairs.begin(), key_value_pairs.end()}`.
  /// Its unspecified which pair is inserted in case of names not being unique.
  template <typename InputIt>
  HeaderMap(InputIt first, InputIt last);

  /// Destructor
  ~HeaderMap();

  /// Copy constructor
  HeaderMap(const HeaderMap& other);
  /// Move constructor
  HeaderMap(HeaderMap&& other) noexcept;
  /// Copy assignment operator
  HeaderMap& operator=(const HeaderMap& other);
  /// Move assignment operator
  HeaderMap& operator=(HeaderMap&& other) noexcept;

  /// Non-binding call to reserve `capacity` elements for internal storage.
  void reserve(std::size_t capacity);
  /// Returns the amount of name-value-pairs being stored.
  std::size_t size() const noexcept;
  /// Return true if no name-value-pairs are being stored, false otherwise.
  bool empty() const noexcept;
  /// Removes all the key-value-pairs being stored,
  /// doesn't shrink underlying storage.
  void clear();

  /// Returns 1 if the key exists, 0 otherwise.
  std::size_t count(std::string_view key) const noexcept;
  /// @overload
  std::size_t count(const PredefinedHeader& key) const noexcept;

  template <std::size_t Size>
  std::size_t count(const char (&)[Size]) const noexcept {
    ReportMisuse<Size>();
  }

  /// Returns true if the key exists, false otherwise.
  bool contains(std::string_view key) const noexcept;
  /// @overload
  bool contains(const PredefinedHeader& key) const noexcept;

  template <std::size_t Size>
  bool contains(const char (&)[Size]) const noexcept {
    ReportMisuse<Size>();
  }

  /// If the key is present, returns reference to it's header value,
  /// otherwise inserts a pair (key, "") and returns reference
  /// to newly inserted empty string.
  /// In an insertion took place, key is moved-out,
  /// otherwise key is left unchanged.
  std::string& operator[](std::string&& key);
  /// If the key is present, returns reference to it's header value,
  /// otherwise inserts a pair (key, "") and returns reference
  /// to newly inserted empty string.
  std::string& operator[](std::string_view key);
  /// @overload
  std::string& operator[](const PredefinedHeader& key);

  template <std::size_t Size>
  std::string& operator[](const char (&)[Size]) {
    ReportMisuse<Size>();
  }

  /// If the key is present, returns iterator to its name-value pair,
  /// otherwise return end().
  Iterator find(std::string_view key) noexcept;
  /// @overload
  ConstIterator find(std::string_view key) const noexcept;

  /// If the key is present, returns iterator to its name-value pair,
  /// otherwise return end().
  Iterator find(const PredefinedHeader& key) noexcept;
  /// @overload
  ConstIterator find(const PredefinedHeader& key) const noexcept;

  template <std::size_t Size>
  Iterator find(const char (&)[Size]) noexcept;

  template <std::size_t Size>
  ConstIterator find(const char (&)[Size]) const noexcept;

  /// If the key is already present in the map, does nothing.
  /// Otherwise inserts a pair of key and in-place constructed value.
  template <typename... Args>
  void emplace(std::string_view key, Args&&... args) {
    Emplace(std::move(key), std::forward<Args>(args)...);
  }

  /// If the key is already present in the map, does nothing.
  /// Otherwise inserts a pair of key and inplace constructed value.
  template <typename... Args>
  void emplace(std::string key, Args&&... args) {
    Emplace(std::move(key), std::forward<Args>(args)...);
  }

  /// If the key is already present in the map, does nothing.
  /// Otherwise inserts a pair of key and inplace constructed value.
  template <typename... Args>
  void try_emplace(std::string key, Args&&... args) {
    Emplace(std::move(key), std::forward<Args>(args)...);
  }

  /// For every iterator it in [first, last) inserts *it.
  /// Its unspecified which pair is inserted in case of names not being unique.
  template <typename InputIt>
  void insert(InputIt first, InputIt last);

  /// If kvp.first is already present in the map, does nothing,
  /// otherwise inserts the pair into container.
  void insert(const std::pair<std::string, std::string>& kvp);
  /// If kvp.first is already present in the map, does nothing,
  /// otherwise inserts the pair into container.
  void insert(std::pair<std::string, std::string>&& kvp);

  /// If key is already present in the map, changes corresponding header value
  /// to provided value, otherwise inserts the pair into container.
  void insert_or_assign(std::string key, std::string value);
  /// @overload
  void insert_or_assign(const PredefinedHeader& key, std::string value);

  /// If key is already present in the map, appends ",{value}" to the
  /// corresponding header value, otherwise inserts the pair into container.
  void InsertOrAppend(std::string key, std::string value);
  /// @overload
  void InsertOrAppend(const PredefinedHeader& key, std::string value);

  /// Erases the pair to which the iterator points, returns iterator following
  /// the last removed element.
  /// Iterators/pointers to erased value and `begin` are invalidated.
  Iterator erase(Iterator it);
  /// Erases the pair to which the iterator points, returns iterator following
  /// the last removed element.
  /// Iterators/pointers to erased value and `begin` are invalidated.
  Iterator erase(ConstIterator it);

  /// If the key is present in container, behaves exactly like erase(Iterator).
  /// Otherwise does nothing and returns `end()`.
  Iterator erase(std::string_view key);
  /// @overload
  Iterator erase(const PredefinedHeader& key);

  template <std::size_t Size>
  Iterator erase(const char (&)[Size]);

  /// If the key is present in container, returns reference to its header value,
  /// otherwise throws std::out_of_range.
  std::string& at(std::string_view key);
  /// @overload
  std::string& at(const PredefinedHeader& key);
  /// If the key is present in container, returns reference to its header value,
  /// otherwise throws std::out_of_range.
  const std::string& at(std::string_view key) const;
  /// @overload
  const std::string& at(const PredefinedHeader& key) const;

  template <std::size_t Size>
  std::string& at(const char (&)[Size]) {
    ReportMisuse<Size>();
  }
  template <std::size_t Size>
  const std::string& at(const char (&)[Size]) const {
    ReportMisuse<Size>();
  }

  /// Returns an iterator to the first name-value-pair being stored.
  Iterator begin() noexcept;
  /// Returns an iterator to the first name-value-pair being stored.
  ConstIterator begin() const noexcept;
  /// Returns an iterator to the first name-value-pair being stored.
  ConstIterator cbegin() const noexcept;

  /// Returns an iterator to the end (valid but not dereferenceable).
  Iterator end() noexcept;
  /// Returns an iterator to the end (valid but not dereferenceable).
  ConstIterator end() const noexcept;
  /// Returns an iterator to the end (valid but not dereferenceable).
  ConstIterator cend() const noexcept;

  /// Returns true if `other` contains exactly the same set of name-value-pairs,
  /// false otherwise.
  bool operator==(const HeaderMap& other) const noexcept;

  /// Appends container content in http headers format to provided buffer,
  /// that is appends
  /// @code
  /// header1: value1\r\n
  /// header2: value2\r\n
  /// ...
  /// @endcode
  /// resizing buffer as needed.
  void OutputInHttpFormat(std::string& buffer) const;

 private:
  friend class TestsHelper;

  template <typename KeyType, typename... Args>
  void Emplace(KeyType&& key, Args&&... args);

  template <std::size_t Size>
  [[noreturn]] static void ReportMisuse();

  utils::FastPimpl<header_map::Map, 272, 8> impl_;
};

template <typename InputIt>
HeaderMap::HeaderMap(InputIt first, InputIt last) : HeaderMap{} {
  insert(first, last);
}

template <typename InputIt>
void HeaderMap::insert(InputIt first, InputIt last) {
  for (; first != last; ++first) {
    insert(*first);
  }
}

template <typename KeyType, typename... Args>
void HeaderMap::Emplace(KeyType&& key, Args&&... args) {
  static_assert(std::is_rvalue_reference_v<decltype(key)>);
  // This is a private function, and we know what we are doing here.
  // NOLINTNEXTLINE(bugprone-move-forwarding-reference)
  auto& value = operator[](std::move(key));
  if (value.empty()) {
    value = std::string{std::forward<Args>(args)...};
  }
}

template <std::size_t Size>
void HeaderMap::ReportMisuse() {
  static_assert(!Size,
                "Please create a 'constexpr PredefinedHeader' and use that "
                "instead of passing header name as a string literal.");
}

namespace header_map {

class MapEntry final {
 public:
  MapEntry();
  ~MapEntry();

  MapEntry(std::string&& key, std::string&& value);

  MapEntry(const MapEntry& other);
  MapEntry& operator=(const MapEntry& other);
  MapEntry(MapEntry&& other) noexcept;
  MapEntry& operator=(MapEntry&& other) noexcept;

  std::pair<const std::string, std::string>& Get();
  const std::pair<const std::string, std::string>& Get() const;

  std::pair<std::string, std::string>& GetMutable();

  bool operator==(const MapEntry& other) const;

 private:
  // The interface requires std::pair<CONST std::string, std::string>, but we
  // don't want to copy where move would do, so this.
  // Only 'mutable_value' is ever the active member of 'Slot', but we still
  // can access it through `value` due to
  // https://eel.is/c++draft/class.union.general#note-1
  // The idea was taken from abseil:
  // https://github.com/abseil/abseil-cpp/blob/1ae9b71c474628d60eb251a3f62967fe64151bb2/absl/container/internal/container_memory.h#L302
  union Slot {
    Slot();
    ~Slot();

    std::pair<std::string, std::string> mutable_value;
    std::pair<const std::string, std::string> value;
  };

  Slot slot_{};
};

}  // namespace header_map

class HeaderMap::Iterator final {
 public:
  using iterator_category = std::forward_iterator_tag;
  using difference_type = std::ptrdiff_t;
  using value_type = std::pair<const std::string, std::string>;
  using reference = value_type&;
  using const_reference = const value_type&;
  using pointer = value_type*;
  using const_pointer = const value_type*;

  // The underlying iterator is a reversed one to do not invalidate
  // end() on erase - end() is actually an rbegin() of underlying storage and is
  // only invalidated on reallocation.
  using UnderlyingIterator =
      std::vector<header_map::MapEntry>::reverse_iterator;

  Iterator();
  explicit Iterator(UnderlyingIterator it);
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

  bool operator==(const ConstIterator& other) const;

 private:
  friend class HeaderMap::ConstIterator;

  UnderlyingIterator it_{};
};

class HeaderMap::ConstIterator final {
 public:
  using iterator_category = std::forward_iterator_tag;
  using difference_type = std::ptrdiff_t;
  using value_type = std::pair<const std::string, std::string>;
  using reference = const value_type&;
  using const_reference = const value_type&;
  using pointer = const value_type*;
  using const_pointer = const value_type*;

  // The underlying iterator is a reversed one to do not invalidate
  // end() on erase - end() is actually an rbegin() of underlying storage and is
  // only invalidated on reallocation.
  using UnderlyingIterator =
      std::vector<header_map::MapEntry>::const_reverse_iterator;

  ConstIterator();
  explicit ConstIterator(UnderlyingIterator it);
  ~ConstIterator();

  ConstIterator(const ConstIterator& other);
  ConstIterator(ConstIterator&& other) noexcept;
  ConstIterator& operator=(const ConstIterator& other);
  ConstIterator& operator=(ConstIterator&& other) noexcept;

  ConstIterator operator++(int);
  ConstIterator& operator++();

  reference operator*();
  const_reference operator*() const;
  pointer operator->();
  const_pointer operator->() const;

  bool operator==(const ConstIterator& other) const;
  bool operator!=(const ConstIterator& other) const;

  bool operator==(const Iterator& other) const;

 private:
  friend class HeaderMap::Iterator;

  UnderlyingIterator it_{};
};

template <std::size_t Size>
HeaderMap::Iterator HeaderMap::find(const char (&)[Size]) noexcept {
  ReportMisuse<Size>();
}

template <std::size_t Size>
HeaderMap::ConstIterator HeaderMap::find(const char (&)[Size]) const noexcept {
  ReportMisuse<Size>();
}

template <std::size_t Size>
HeaderMap::Iterator HeaderMap::erase(const char (&)[Size]) {
  ReportMisuse<Size>();
}

}  // namespace http::headers

USERVER_NAMESPACE_END
