#pragma once

#include <vector>

#include <userver/server/http/header_map.hpp>

#include <userver/utils/assert.hpp>

#include <server/http/header_map_impl/danger.hpp>
#include <server/http/header_map_impl/traits.hpp>

USERVER_NAMESPACE_BEGIN

namespace server::http::header_map_impl {

struct Pos final {
  // Index in the 'entries' vector
  Traits::Size index;
  // Full hash value for the entry
  Traits::HashValue hash;

  Pos(Traits::Size index, Traits::HashValue hash);
  Pos(std::size_t index, Traits::HashValue hash);
  static Pos None();

  bool IsNone() const noexcept;
  bool IsSome() const noexcept;
};

struct Entry final {
  std::string header_name;
  std::string header_value;

  Entry(std::string&& header_name, std::string&& header_value)
      : header_name{std::move(header_name)},
        header_value{std::move(header_value)} {}
};

static_assert(std::is_same_v<decltype(Entry::header_name), Traits::Key>);
static_assert(std::is_same_v<decltype(Entry::header_value), Traits::Value>);

class Map final {
 public:
  using Iterator = std::vector<Entry>::iterator;
  using ConstIterator = std::vector<Entry>::const_iterator;

  std::size_t Size() const noexcept;
  bool Empty() const noexcept;

  Iterator Find(std::string_view key) const noexcept;

  void Insert(std::string key, std::string value);

  Iterator Begin() const noexcept;
  Iterator End() const noexcept;

 private:
  void ReserveOne();

  void Grow(std::size_t new_capacity);
  void ReinsertEntryInOrder(Pos pos);

  void Rebuild();

  std::size_t Capacity() const noexcept;

  template <typename Fn>
  std::size_t ProbeLoop(std::size_t starting_position, Fn&& probe_body) const;

  Traits::HashValue HashKey(std::string_view key) const noexcept;

  void InsertEntry(Traits::Key&& key, Traits::Value&& value);
  std::size_t DoRobinhoodAtPosition(std::size_t idx, Pos old_pos);

  Pos DoFind(std::string_view key) const noexcept;

  Traits::Size mask_{0};
  std::vector<Pos> indices_;
  std::vector<Entry> entries_;
  Danger danger_;

  // To get non-const iterator from Find
  std::vector<Entry>& entries_ref_{entries_};
};

}  // namespace server::http::header_map_impl

USERVER_NAMESPACE_END
