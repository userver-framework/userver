#pragma once

#include <vector>

#include <boost/container/small_vector.hpp>

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
  Traits::Key header_name;
  Traits::Value header_value;

  Entry(Traits::Key&& header_name, Traits::Value&& header_value)
      : header_name{std::move(header_name)},
        header_value{std::move(header_value)} {}
};

class Map final {
 public:
  using Iterator = std::vector<Entry>::iterator;
  using ConstIterator = std::vector<Entry>::const_iterator;

  std::size_t Size() const noexcept;
  bool Empty() const noexcept;

  Iterator Find(std::string_view key) noexcept;
  ConstIterator Find(std::string_view key) const noexcept;

  Iterator Find(SpecialHeader header) noexcept;
  ConstIterator Find(SpecialHeader header) const noexcept;

  void Insert(std::string&& key, std::string&& value, bool append);
  void Insert(SpecialHeader header, std::string&& value, bool append);

  void Erase(std::string_view key);
  void Erase(SpecialHeader header);

  void Clear();

  Iterator Begin() noexcept;
  ConstIterator Begin() const noexcept;

  Iterator End() noexcept;
  ConstIterator End() const noexcept;

 private:
  void ReserveOne();

  void Grow(std::size_t new_capacity);
  void ReinsertEntryInOrder(Pos pos);

  void Rebuild();

  std::size_t Capacity() const noexcept;

  template <typename Fn>
  std::size_t ProbeLoop(std::size_t starting_position, Fn&& probe_body) const;

  static Traits::HashValue MaskHash(std::size_t hash) noexcept;
  Traits::HashValue HashKey(std::string_view key) const noexcept;
  Traits::HashValue HashKey(SpecialHeader header) const noexcept;

  void InsertEntry(Traits::Key&& key, Traits::Value&& value);
  std::size_t DoRobinhoodAtPosition(std::size_t idx, Pos old_pos);

  struct FindResult final {
    // Index in the 'indices' vector
    Traits::Size probe;
    // Index in the 'entries' vector
    Traits::Size index;

    bool IsSome() const noexcept;

    static FindResult None() noexcept;
  };
  FindResult DoFind(std::string_view key,
                    Traits::HashValue hash) const noexcept;
  void DoInsert(std::string&& key, Traits::HashValue hash, std::string&& value,
                bool append);
  void DoErase(std::string_view key, Traits::HashValue hash);

  Traits::Size mask_{0};
  boost::container::small_vector<Pos, 16> indices_;
  std::vector<Entry> entries_;
  Danger danger_;
};

}  // namespace server::http::header_map_impl

USERVER_NAMESPACE_END
