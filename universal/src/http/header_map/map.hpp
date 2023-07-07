#pragma once

#include <vector>

#include <boost/container/small_vector.hpp>

#include <userver/http/header_map.hpp>
#include <userver/http/predefined_header.hpp>
#include <userver/utils/str_icase.hpp>

#include <http/header_map/danger.hpp>
#include <http/header_map/traits.hpp>

USERVER_NAMESPACE_BEGIN

namespace http::headers {

class TestsHelper;

namespace header_map {

class Pos final {
 public:
  Pos(Traits::Size entries_index, Traits::HashValue hash,
      Traits::HeaderIndex header_index);
  Pos(std::size_t entries_index, Traits::HashValue hash,
      Traits::HeaderIndex header_index);
  static Pos None();

  bool IsNone() const noexcept;
  bool IsSome() const noexcept;

  Traits::Size GetEntriesIndex() const noexcept;
  Traits::HashValue GetHash() const noexcept;
  Traits::HeaderIndex GetHeaderIndex() const noexcept;

 private:
  Traits::Size entries_index_;
  Traits::HashValue hash_;
  // Header index if header is known or some sentinel value otherwise
  Traits::HeaderIndex header_index_;
};

class MaybeOwnedKey final {
 public:
  explicit MaybeOwnedKey(std::string& key);
  explicit MaybeOwnedKey(const PredefinedHeader& key);
  explicit MaybeOwnedKey(std::string_view key);

  std::string_view GetValue() const;

  std::string ExtractValue() &&;

 private:
  std::string_view key_;
  std::string* key_str_if_exists_;
};

class Map final {
 public:
  using Iterator = std::vector<MapEntry>::reverse_iterator;
  using ConstIterator = std::vector<MapEntry>::const_reverse_iterator;

  Map();

  void Reserve(std::size_t capacity);

  std::size_t Size() const noexcept;
  bool Empty() const noexcept;

  Iterator Find(std::string_view key) noexcept;
  ConstIterator Find(std::string_view key) const noexcept;

  Iterator Find(const PredefinedHeader& header) noexcept;
  ConstIterator Find(const PredefinedHeader& header) const noexcept;

  enum class InsertOrModifyOccupiedAction { kAppend, kReplace, kNoop, kAssert };

  Iterator InsertOrModify(MaybeOwnedKey key, std::string&& value,
                          InsertOrModifyOccupiedAction occupied_action);
  Iterator InsertOrModify(const PredefinedHeader& header, std::string&& value,
                          InsertOrModifyOccupiedAction occupied_action);

  Iterator Erase(std::string_view key);
  Iterator Erase(const PredefinedHeader& header);

  void Clear();

  Iterator Begin() noexcept;
  ConstIterator Begin() const noexcept;

  Iterator End() noexcept;
  ConstIterator End() const noexcept;

  bool operator==(const Map& other) const noexcept;

  void OutputInHttpFormat(std::string& buffer) const;

 private:
  friend class http::headers::TestsHelper;

  void ReserveOne();

  void Grow(std::size_t new_capacity);
  void ReinsertPositionInOrder(Pos pos);

  void Rebuild();

  std::size_t Capacity() const noexcept;

  enum class ProbingAction { kStop, kContinue };

  template <typename Fn>
  std::size_t ProbeLoop(std::size_t positions_idx, Fn&& probe_body) const;

  Traits::HashValue HashKey(std::string_view key) const noexcept;
  Traits::HashValue HashKey(const PredefinedHeader& header) const noexcept;

  Traits::HeaderIndex InsertEntry(std::string&& key, std::string&& value);
  std::size_t DoRobinhoodAtPosition(std::size_t idx, Pos old_pos);

  struct FindResult final {
    Traits::Size positions_index;
    Traits::Size entries_index;

    bool IsSome() const noexcept;

    static FindResult None() noexcept;
  };
  FindResult DoFind(std::string_view key, Traits::HashValue hash,
                    int header_index) const noexcept;
  Iterator DoInsertOrModify(MaybeOwnedKey key, Traits::HashValue hash,
                            std::string&& value,
                            InsertOrModifyOccupiedAction occupied_action);
  Iterator DoErase(std::string_view key, Traits::HashValue hash);

  static Iterator ToReverseIterator(std::vector<MapEntry>::iterator it);
  static ConstIterator ToReverseIterator(
      std::vector<MapEntry>::const_iterator it);

  static bool AreValuesICaseEqual(std::string_view lhs,
                                  std::string_view rhs) noexcept;

  std::size_t DebugMaxDisplacement() const;
  void DebugForceIntoRed();

  static constexpr bool IsPowerOf2(std::size_t n) noexcept {
    return (n & (n - 1)) == 0;
  }

  static constexpr std::size_t kOnStackPositionsCount = 32;

  Traits::Size mask_{0};
  boost::container::small_vector<Pos, kOnStackPositionsCount> positions_;
  std::vector<MapEntry> entries_;
  Danger danger_;
};

}  // namespace header_map

}  // namespace http::headers

USERVER_NAMESPACE_END
