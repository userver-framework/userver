#include <http/header_map/map.hpp>

// Inspired by
// https://github.com/hyperium/http/blob/f0ba97fe20054d6ef83834f92a0842be947996bd/src/header/map.rs

USERVER_NAMESPACE_BEGIN

namespace http::headers::header_map {

namespace {

// Constants related to detecting DOS attacks.
//
// Displacement is the number of entries that get shifted when inserting a new
// value. Forward shift is how far the entry gets stored from the ideal
// position.
//
// The current constant values were picked from another implementation. It could
// be that there are different values better suited to the header map case.
constexpr std::size_t kDisplacementThreshold = 128;
constexpr std::size_t kForwardShiftThreshold = 128;

// The default strategy for handling the yellow danger state is to increase the
// header map capacity in order to (hopefully) reduce the number of collisions.
// If growing the hash map would cause the load factor to drop bellow this
// threshold, then instead of growing, the map is switched to the red
// danger state and safe hashing is used instead.
constexpr float kLoadFactorThreshold = 0.2;

std::size_t DesiredPos(Traits::Size mask, Traits::HashValue hash) noexcept {
  return static_cast<std::size_t>(hash & mask);
}

// how much to the right shall we move from hash to reach current,
// wrapping clockwise distance
std::size_t ProbeDistance(Traits::Size mask, Traits::HashValue hash,
                          std::size_t current) noexcept {
  return (current - DesiredPos(mask, hash)) & mask;
}

// positions capacity -> entries capacity
std::size_t UsableCapacity(std::size_t capacity) noexcept {
  // max load factor is fixed as 0.75, so X slots for hashes = 0.75X slots for
  // entries.
  return capacity - capacity / 4;
}

Traits::HashValue MaskHash(std::size_t hash) noexcept {
  constexpr std::size_t kMask = Traits::kMaxSize - 1;

  return static_cast<Traits::HashValue>(hash & kMask);
}

constexpr Traits::Size kNoneIndex = std::numeric_limits<Traits::Size>::max();

}  // namespace

Pos::Pos(Traits::Size entries_index, Traits::HashValue hash,
         Traits::HeaderIndex header_index)
    : entries_index_{entries_index}, hash_{hash}, header_index_{header_index} {}

Pos::Pos(std::size_t entries_index, Traits::HashValue hash,
         Traits::HeaderIndex header_index)
    : Pos{static_cast<Traits::Size>(entries_index), hash, header_index} {
  UASSERT(entries_index < Traits::kMaxSize);
}

Pos Pos::None() { return Pos{kNoneIndex, 0, 0}; }

bool Pos::IsNone() const noexcept { return entries_index_ == kNoneIndex; }

bool Pos::IsSome() const noexcept { return !IsNone(); }

Traits::Size Pos::GetEntriesIndex() const noexcept { return entries_index_; }

Traits::HashValue Pos::GetHash() const noexcept { return hash_; }

Traits::HeaderIndex Pos::GetHeaderIndex() const noexcept {
  return header_index_;
}

namespace {

#ifdef __cpp_lib_is_layout_compatible
static_assert(
    std::is_layout_compatible_v<std::pair<std::string, std::string>,
                                std::pair<const std::string, std::string>>);
#else
template <typename Pair>
struct OffsetOf final {
  static_assert(std::is_standard_layout_v<Pair>);

  static constexpr size_t kFirst = offsetof(Pair, first);
  static constexpr size_t kSecond = offsetof(Pair, second);
};

template <typename K, typename V>
struct IsLayoutCompatible final {
 private:
  struct MyPair {
    K first;
    V second;
  };

  template <class P>
  static constexpr bool kLayoutCompatible =
      std::is_standard_layout_v<P> &&                     //
      sizeof(P) == sizeof(MyPair) &&                      //
      OffsetOf<P>::kFirst == OffsetOf<MyPair>::kFirst &&  //
      OffsetOf<P>::kSecond == OffsetOf<MyPair>::kSecond;

 public:
  // Whether pair<const K, V> and pair<K, V> are layout-compatible. If they are,
  // then it is safe to store them in a union and read from either.
  static constexpr bool value = std::is_standard_layout<MyPair>() &&
                                OffsetOf<MyPair>::kFirst == 0 &&
                                kLayoutCompatible<std::pair<K, V>> &&
                                kLayoutCompatible<std::pair<const K, V>>;
};
static_assert(IsLayoutCompatible<std::string, std::string>::value);
#endif

}  // namespace

// ----------------------------------------------------------------------------

MapEntry::MapEntry() {
  new (&slot_.mutable_value) std::pair<std::string, std::string>{};
}
MapEntry::~MapEntry() { std::destroy_at(&slot_.mutable_value); }

MapEntry::MapEntry(std::string&& key, std::string&& value) {
  new (&slot_.mutable_value)
      std::pair<std::string, std::string>{std::move(key), std::move(value)};
}

MapEntry::MapEntry(const MapEntry& other) {
  new (&slot_.mutable_value)
      std::pair<std::string, std::string>{other.slot_.mutable_value};
}

MapEntry& MapEntry::operator=(const MapEntry& other) {
  if (this != &other) {
    slot_.mutable_value = other.slot_.mutable_value;
  }

  return *this;
}

MapEntry::MapEntry(MapEntry&& other) noexcept {
  new (&slot_.mutable_value)
      std::pair<std::string, std::string>{std::move(other.slot_.mutable_value)};
}

MapEntry& MapEntry::operator=(MapEntry&& other) noexcept {
  slot_.mutable_value = std::move(other.slot_.mutable_value);

  return *this;
}

std::pair<const std::string, std::string>& MapEntry::Get() {
  return slot_.value;
}

const std::pair<const std::string, std::string>& MapEntry::Get() const {
  return slot_.value;
}

std::pair<std::string, std::string>& MapEntry::GetMutable() {
  return slot_.mutable_value;
}

bool MapEntry::operator==(const MapEntry& other) const {
  return Get() == other.Get();
}

// Tidy really wants =default with these 2, but it's a union with
// non-trivial objects in it, so both ctor and dtor are deleted by default.
// NOLINTNEXTLINE(hicpp-use-equals-default,modernize-use-equals-default)
MapEntry::Slot::Slot() {}
// NOLINTNEXTLINE(hicpp-use-equals-default,modernize-use-equals-default)
MapEntry::Slot::~Slot() { static_assert(std::is_standard_layout_v<Slot>); }

MaybeOwnedKey::MaybeOwnedKey(std::string& key)
    : key_{key}, key_str_if_exists_{&key} {}
MaybeOwnedKey::MaybeOwnedKey(const PredefinedHeader& key)
    : MaybeOwnedKey{std::string_view{key}} {}
MaybeOwnedKey::MaybeOwnedKey(std::string_view key)
    : key_{key}, key_str_if_exists_{nullptr} {}

std::string_view MaybeOwnedKey::GetValue() const { return key_; }

std::string MaybeOwnedKey::ExtractValue() && {
  if (key_str_if_exists_) {
    UASSERT(key_ == *key_str_if_exists_);
    return std::move(*key_str_if_exists_);
  }

  return std::string{key_};
}

bool Map::FindResult::IsSome() const noexcept {
  return entries_index != kNoneIndex;
}

Map::FindResult Map::FindResult::None() noexcept {
  return FindResult{0, kNoneIndex};
}

Map::Map() { static_assert(IsPowerOf2(kOnStackPositionsCount)); }

void Map::Reserve(std::size_t capacity) {
  // We don't touch positions here because:
  // 1. it's a small vector with capacity probably big enough anyway
  // 2. we know better how it should be managed
  //
  // for entries tho we can reserve, why not
  entries_.reserve(capacity);
}

std::size_t Map::Size() const noexcept { return entries_.size(); }

bool Map::Empty() const noexcept { return entries_.empty(); }

void Map::Grow(std::size_t new_capacity) {
  UASSERT(IsPowerOf2(new_capacity));

  if (new_capacity > Traits::kMaxSize) {
    throw HeaderMap::TooManyHeadersException{
        "HeaderMap reached its maximum capacity"};
  }

  // find the first ideally placed pos - start of a bucket
  std::size_t first_ideal = 0;
  for (std::size_t i = 0; i < positions_.size(); ++i) {
    if (positions_[i].IsSome()) {
      if (ProbeDistance(mask_, positions_[i].GetHash(), i) == 0) {
        first_ideal = i;
        break;
      }
    }
  }

  auto old_positions = positions_;
  positions_.assign(new_capacity, Pos::None());

  mask_ = static_cast<Traits::Size>(new_capacity - 1);

  // visit the entries in an order where we can simply reinsert them
  // into 'positions' without any bucket stealing
  for (std::size_t i = first_ideal; i < old_positions.size(); ++i) {
    ReinsertPositionInOrder(old_positions[i]);
  }

  for (std::size_t i = 0; i < first_ideal; ++i) {
    ReinsertPositionInOrder(old_positions[i]);
  }
}

void Map::ReserveOne() {
  const auto len = entries_.size();

  if (danger_.IsYellow()) {
    // it can never be, but why not to check
    UASSERT(!positions_.empty());
    // Yea, float. Totally not a hot path, so why bother with math.
    const auto load_factor =
        static_cast<float>(entries_.size()) / positions_.size();

    if (load_factor >= kLoadFactorThreshold) {
      // load factor is still sane, lets go green, grow and hope for the best
      danger_.ToGreen();

      const auto new_capacity = positions_.size() * 2;

      Grow(new_capacity);
    } else {
      // well, we are under attack it seems, switch to red and rebuild
      danger_.ToRed();

      Rebuild();
    }
  } else if (len == Capacity()) {
    if (len == 0) {
      const auto new_raw_capacity = kOnStackPositionsCount;
      mask_ = new_raw_capacity - 1;
      positions_.assign(new_raw_capacity, Pos::None());
      // Wild guess, but we definitely don't want to reserve too much here,
      // because HeaderMap with just a few headers is a pretty common scenario.
      entries_.reserve(kOnStackPositionsCount / 2);
    } else {
      const auto raw_capacity = positions_.size();
      Grow(raw_capacity * 2);
    }
  }
}

void Map::ReinsertPositionInOrder(Pos pos) {
  if (pos.IsSome()) {
    const auto probe = DesiredPos(mask_, pos.GetHash());

    ProbeLoop(probe, [this, pos](std::size_t positions_idx) {
      if (positions_[positions_idx].IsNone()) {
        positions_[positions_idx] = pos;
        return ProbingAction::kStop;
      } else {
        return ProbingAction::kContinue;
      }
    });
  }
}

void Map::Rebuild() {
  UASSERT(danger_.IsRed());

  // could be done in a more performant way (no need to rebuild entries vector),
  // but totally not a hot path: happens at most once per map lifetime, in
  // trusted environment never happens at all.
  // Let's just do something really simple

  positions_.assign(positions_.size(), Pos::None());

  std::vector<MapEntry> entries;
  std::swap(entries_, entries);
  entries_.reserve(entries.size());

  for (auto& entry : entries) {
    auto& value = entry.GetMutable();
    InsertOrModify(MaybeOwnedKey{value.first}, std::move(value.second),
                   InsertOrModifyOccupiedAction::kAssert);
  }
}

std::size_t Map::Capacity() const noexcept {
  return UsableCapacity(positions_.size());
}

template <typename Fn>
std::size_t Map::ProbeLoop(std::size_t positions_idx, Fn&& probe_body) const {
  UASSERT(!positions_.empty());

  // goes clockwise until probe_body stops the loop, hence positions vector
  // should be non-empty and probe_body should eventually terminate
  while (true) {
    if (positions_idx < positions_.size()) {
      if (probe_body(positions_idx) == ProbingAction::kStop) {
        return positions_idx;
      }
      ++positions_idx;
    } else {
      positions_idx = 0;
    }
  }
}

Traits::HashValue Map::HashKey(std::string_view key) const noexcept {
  return MaskHash(danger_.HashKey(key));
}

Traits::HashValue Map::HashKey(const PredefinedHeader& header) const noexcept {
  // this is basically why we have a custom hashtable implemented
  return MaskHash(danger_.HashKey(header));
}

Traits::HeaderIndex Map::InsertEntry(std::string&& key, std::string&& value) {
  // we could potentially restructure the code a bit, so instead of hashing
  // and then at some point looking for index here,
  // we look for index first, and if it's found - that means we already know the
  // hash (the header name is known upfront, thus the hash is known as well).
  // Clobbers interface/implementation a bit, and
  // we're not sure if its worth it, since default hash is decently fast anyway.
  const auto header_index = impl::GetHeaderIndexForInsertion(key);

  entries_.emplace_back(std::move(key), std::move(value));

  return header_index;
}

Map::ConstIterator Map::Find(std::string_view key) const noexcept {
  const auto pos = DoFind(key, HashKey(key), 0);

  return pos.IsSome() ? ToReverseIterator(entries_.cbegin() + pos.entries_index)
                      : End();
}

Map::Iterator Map::Find(std::string_view key) noexcept {
  const auto pos = DoFind(key, HashKey(key), 0);

  return pos.IsSome() ? ToReverseIterator(entries_.begin() + pos.entries_index)
                      : End();
}

Map::ConstIterator Map::Find(const PredefinedHeader& header) const noexcept {
  const auto pos = DoFind(header.name, HashKey(header), header.header_index);

  return pos.IsSome() ? ToReverseIterator(entries_.cbegin() + pos.entries_index)
                      : End();
}

Map::Iterator Map::Find(const PredefinedHeader& header) noexcept {
  const auto pos = DoFind(header.name, HashKey(header), header.header_index);

  return pos.IsSome() ? ToReverseIterator(entries_.begin() + pos.entries_index)
                      : End();
}

Map::FindResult Map::DoFind(std::string_view key, Traits::HashValue hash,
                            int header_index) const noexcept {
  // ProbeLoop doesn't work with empty positions
  if (positions_.empty()) {
    return FindResult::None();
  }

  const auto probe = DesiredPos(mask_, hash);

  std::size_t dist = 0;

  auto res = FindResult::None();
  ProbeLoop(
      probe, [this, key, hash, header_index, &dist, &res](std::size_t idx) {
        if (positions_[idx].IsNone()) {
          // the cluster ended, no luck
          return ProbingAction::kStop;
        }

        if (dist > ProbeDistance(mask_, positions_[idx].GetHash(), idx)) {
          // we've checked for more values than this pos displacement, our hash
          // is not present
          return ProbingAction::kStop;
        } else if (positions_[idx].GetHash() == hash &&
                   (header_index == positions_[idx].GetHeaderIndex() ||
                    AreValuesICaseEqual(
                        entries_[positions_[idx].GetEntriesIndex()].Get().first,
                        key))) {
          res = FindResult{static_cast<Traits::Size>(idx),
                           positions_[idx].GetEntriesIndex()};
          return ProbingAction::kStop;
        }

        ++dist;
        return ProbingAction::kContinue;
      });

  return res;
}

Map::Iterator Map::InsertOrModify(
    MaybeOwnedKey key, std::string&& value,
    InsertOrModifyOccupiedAction occupied_action) {
  return DoInsertOrModify(key, HashKey(key.GetValue()), std::move(value),
                          occupied_action);
}

Map::Iterator Map::InsertOrModify(
    const PredefinedHeader& header, std::string&& value,
    InsertOrModifyOccupiedAction occupied_action) {
  return DoInsertOrModify(MaybeOwnedKey{header}, HashKey(header),
                          std::move(value), occupied_action);
}

Map::Iterator Map::DoInsertOrModify(
    MaybeOwnedKey key, Traits::HashValue hash, std::string&& value,
    InsertOrModifyOccupiedAction occupied_action) {
  ReserveOne();

  const auto perform_occupied = [this, occupied_action](std::size_t entries_idx,
                                                        std::string&& value) {
    auto& header_value = entries_[entries_idx].Get().second;

    switch (occupied_action) {
      case InsertOrModifyOccupiedAction::kAppend: {
        header_value += ',';
        header_value += value;
        break;
      }
      case InsertOrModifyOccupiedAction::kReplace: {
        header_value = std::move(value);
        break;
      }
      case InsertOrModifyOccupiedAction::kNoop: {
        break;
      }
      case InsertOrModifyOccupiedAction::kAssert: {
        UASSERT_MSG(false,
                    "This execution path shouldn't hit occupied branch in "
                    "InsertOrModify");
// dead code in debug otherwise
#ifdef NDEBUG
        break;
#endif
      }
    }
  };

  const auto perform_robinhood =
      [this, hash](std::size_t dist, std::size_t positions_idx,
                   std::string&& key, std::string&& value) {
        const auto entries_index = entries_.size();
        const auto header_index = InsertEntry(std::move(key), std::move(value));

        const auto num_displaced = DoRobinhoodAtPosition(
            positions_idx, Pos{entries_index, hash, header_index});

        if (dist >= kForwardShiftThreshold ||
            num_displaced >= kDisplacementThreshold) {
          danger_.ToYellow();
        }
      };

  const auto perform_vacant = [this, hash](
                                  std::size_t dist, std::size_t positions_idx,
                                  std::string&& key, std::string&& value) {
    const auto index = entries_.size();
    const auto header_index = InsertEntry(std::move(key), std::move(value));
    positions_[positions_idx] = Pos{index, hash, header_index};

    if (dist >= kForwardShiftThreshold) {
      danger_.ToYellow();
    }
  };

  std::size_t dist = 0;
  auto inserter = [this, key, hash, &value,  // comment for cleaner formatting
                   &perform_occupied, &perform_robinhood, &perform_vacant,
                   &dist](std::size_t positions_idx) mutable {
    if (positions_[positions_idx].IsSome()) {
      const auto their_dist = ProbeDistance(
          mask_, positions_[positions_idx].GetHash(), positions_idx);

      if (dist > their_dist) {
        perform_robinhood(dist, positions_idx, std::move(key).ExtractValue(),
                          std::move(value));

        return ProbingAction::kStop;
      } else if (positions_[positions_idx].GetHash() == hash &&
                 AreValuesICaseEqual(
                     entries_[positions_[positions_idx].GetEntriesIndex()]
                         .Get()
                         .first,
                     key.GetValue())) {
        perform_occupied(positions_[positions_idx].GetEntriesIndex(),
                         std::move(value));

        return ProbingAction::kStop;
      }
    } else {
      perform_vacant(dist, positions_idx, std::move(key).ExtractValue(),
                     std::move(value));

      return ProbingAction::kStop;
    }

    ++dist;
    return ProbingAction::kContinue;
  };

  const auto resulting_positions_idx =
      ProbeLoop(DesiredPos(mask_, hash), inserter);
  return ToReverseIterator(
      entries_.begin() + positions_[resulting_positions_idx].GetEntriesIndex());
}

Map::Iterator Map::Erase(std::string_view key) {
  return DoErase(key, HashKey(key));
}

Map::Iterator Map::Erase(const PredefinedHeader& header) {
  return DoErase(header.name, HashKey(header));
}

Map::Iterator Map::DoErase(std::string_view key, Traits::HashValue hash) {
  const auto pos = DoFind(key, hash, 0);
  if (!pos.IsSome()) {
    return End();
  }

  UASSERT(!entries_.empty());

  const auto entries_index = pos.entries_index;
  positions_[pos.positions_index] = Pos::None();
  if (std::size_t{entries_index} + 1 != entries_.size()) {
    entries_[entries_index] = std::move(entries_.back());
  }
  entries_.pop_back();

  // correct index that points to the entry that had to swap places
  if (entries_index < entries_.size()) {
    // was not last element
    // examine new element in `entries_index` and find it in positions
    const auto entry_hash = HashKey(entries_[entries_index].Get().first);
    const auto probe = DesiredPos(mask_, entry_hash);

    ProbeLoop(
        probe, [this, entries_index, entry_hash](std::size_t positions_idx) {
          if (positions_[positions_idx].IsSome() &&
              positions_[positions_idx].GetEntriesIndex() >= entries_.size()) {
            // found it
            UASSERT(positions_[positions_idx].GetHash() == entry_hash);

            positions_[positions_idx] =
                Pos{entries_index, entry_hash,
                    positions_[positions_idx].GetHeaderIndex()};
            return ProbingAction::kStop;
          } else {
            return ProbingAction::kContinue;
          }
        });
  }

  // backward shift deletion in positions
  // after probe, shift all non-ideally placed positions backward
  if (!entries_.empty()) {
    auto last_probe = pos.positions_index;
    ProbeLoop(last_probe + 1, [this, &last_probe](std::size_t idx) {
      if (positions_[idx].IsSome()) {
        if (ProbeDistance(mask_, positions_[idx].GetHash(), idx) > 0) {
          positions_[last_probe] = std::exchange(positions_[idx], Pos::None());
        } else {
          return ProbingAction::kStop;
        }
      } else {
        return ProbingAction::kStop;
      }

      last_probe = idx;
      return ProbingAction::kContinue;
    });
  }

  // our "begin" is reversed, so if we deleted the last element from the
  // entries vector, we return "Begin", which is actually a rbegin.
  return entries_index < entries_.size()
             ? ToReverseIterator(entries_.begin() + entries_index)
             : Begin();
}

void Map::Clear() {
  positions_.assign(positions_.size(), Pos::None());

  entries_.clear();

  // reset the hasher
  danger_ = decltype(danger_){};
}

std::size_t Map::DoRobinhoodAtPosition(std::size_t idx, Pos old_pos) {
  std::size_t num_displaced = 0;
  // this always terminates because our load factor is <= 0.75
  ProbeLoop(idx, [this, &num_displaced, old_pos](std::size_t idx) mutable {
    auto& pos = positions_[idx];
    if (pos.IsNone()) {
      pos = old_pos;
      return ProbingAction::kStop;
    } else {
      ++num_displaced;
      std::swap(pos, old_pos);
      return ProbingAction::kContinue;
    }
  });

  return num_displaced;
}

Map::Iterator Map::Begin() noexcept { return entries_.rbegin(); }
Map::ConstIterator Map::Begin() const noexcept { return entries_.crbegin(); }

Map::Iterator Map::End() noexcept { return entries_.rend(); }
Map::ConstIterator Map::End() const noexcept { return entries_.crend(); }

bool Map::operator==(const Map& other) const noexcept {
  if (Size() != other.Size()) {
    return false;
  }

  for (const auto& entry : entries_) {
    const auto other_it = other.Find(entry.Get().first);
    if (other_it == other.End() ||
        !AreValuesICaseEqual(other_it->Get().second, entry.Get().second)) {
      return false;
    }
  }

  return true;
}

void Map::OutputInHttpFormat(std::string& buffer) const {
  static constexpr std::string_view kCrlf = "\r\n";
  static constexpr std::string_view kKeyValueHeaderSeparator = ": ";

  const auto old_buffer_size = buffer.size();
  const auto amount_to_add = [this] {
    std::size_t total = 0;
    for (const auto& entry : entries_) {
      total += entry.Get().first.size() + kKeyValueHeaderSeparator.size() +
               entry.Get().second.size() + kCrlf.size();
    }
    return total;
  }();

  // one day we will have resize_and_overwrite, one day.. (in C++23)
  // https://www.open-std.org/jtc1/sc22/wg21/docs/papers/2021/p1072r8.html#examples
  //
  // anyway, compiler doesn't really optimize the pattern we have here that well
  // (https://godbolt.org/z/61Eb4dh7z, call + memcpy (coming from append) for
  // 2 constexpr symbols, :/), so doing it this way is measurably faster.
  buffer.resize(old_buffer_size + amount_to_add);

  char* append_position = buffer.data() + old_buffer_size;
  const auto append = [&append_position](std::string_view what) {
    std::memcpy(append_position, what.data(), what.size());
    append_position += what.size();
  };

  for (const auto& entry : entries_) {
    const auto& [name, value] = entry.Get();
    append(name);
    append(kKeyValueHeaderSeparator);
    append(value);
    append(kCrlf);
  }
  UASSERT(buffer.size() == old_buffer_size + amount_to_add);
}

Map::Iterator Map::ToReverseIterator(std::vector<MapEntry>::iterator it) {
  static_assert(!std::is_same_v<Iterator, decltype(it)>);

  return Iterator{++it /* ++ because reversed */};
}

Map::ConstIterator Map::ToReverseIterator(
    std::vector<MapEntry>::const_iterator it) {
  static_assert(!std::is_same_v<ConstIterator, decltype(it)>);

  return ConstIterator{++it /* ++ because reversed */};
}

bool Map::AreValuesICaseEqual(std::string_view lhs,
                              std::string_view rhs) noexcept {
  return utils::StrIcaseEqual{}(lhs, rhs);
}

std::size_t Map::DebugMaxDisplacement() const {
  std::size_t max_displacement = 0;
  if (Empty()) {
    return max_displacement;
  }

  for (std::size_t idx = 0; idx < positions_.size(); ++idx) {
    const auto& pos = positions_[idx];
    if (pos.IsSome()) {
      max_displacement =
          std::max(max_displacement, ProbeDistance(mask_, pos.GetHash(), idx));
    }
  }

  return max_displacement;
}

void Map::DebugForceIntoRed() {
  if (!danger_.IsRed()) {
    danger_.ToYellow();
    danger_.ToRed();
    Rebuild();
  }
}
}  // namespace http::headers::header_map

USERVER_NAMESPACE_END
