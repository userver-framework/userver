#include <server/http/header_map_impl/map.hpp>

// Shameless adaptation of
// https://github.com/hyperium/http/blob/f0ba97fe20054d6ef83834f92a0842be947996bd/src/header/map.rs

#include <vector>

#include <userver/utils/assert.hpp>

#include <server/http/header_map_impl/traits.hpp>

USERVER_NAMESPACE_BEGIN

namespace server::http::header_map_impl {

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
constexpr std::size_t kForwardShiftThreshold = 512;

// The default strategy for handling the yellow danger state is to increase the
// header map capacity in order to (hopefully) reduce the number of collisions.
// If growing the hash map would cause the load factor to drop bellow this
// threshold, then instead of growing, the headermap is switched to the red
// danger state and safe hashing is used instead.
constexpr float kLoadFactorThreshold = 0.2;

}  // namespace

Pos::Pos(Traits::Size index, Traits::HashValue hash)
    : index{index}, hash{hash} {}

Pos::Pos(std::size_t index, Traits::HashValue hash)
    : Pos{static_cast<Traits::Size>(index), hash} {
  UASSERT(index < Traits::kMaxSize);
}

Pos Pos::None() { return Pos{std::numeric_limits<Traits::Size>::max(), 0}; }

bool Pos::IsNone() const noexcept {
  return index == std::numeric_limits<Traits::Size>::max();
}

bool Pos::IsSome() const noexcept { return !IsNone(); }

std::size_t DesiredPos(Traits::Size mask, Traits::HashValue hash) noexcept {
  return static_cast<std::size_t>(hash & mask);
}

std::size_t ProbeDistance(Traits::Size mask, Traits::HashValue hash,
                          std::size_t current) noexcept {
  return static_cast<std::size_t>((current - DesiredPos(mask, hash)) & mask);
}

std::size_t UsableCapacity(std::size_t capacity) noexcept {
  return capacity - capacity / 4;
}

std::size_t ToRawCapacity(std::size_t capacity) noexcept {
  return capacity + capacity / 3;
}

std::size_t Map::Size() const noexcept { return entries_.size(); }

bool Map::Empty() const noexcept { return entries_.empty(); }

void Map::Grow(std::size_t new_capacity) {
  UINVARIANT(new_capacity <= Traits::kMaxSize, "Requested capacity too large");

  std::size_t first_ideal = 0;
  for (std::size_t i = 0; i < indices_.size(); ++i) {
    if (indices_[i].IsSome()) {
      if (ProbeDistance(mask_, indices_[i].hash, i) == 0) {
        first_ideal = i;
        break;
      }
    }
  }

  std::vector<Pos> old_indices(new_capacity, Pos::None());
  std::swap(old_indices, indices_);
  mask_ = static_cast<Traits::Size>(new_capacity - 1);

  for (std::size_t i = first_ideal; i < old_indices.size(); ++i) {
    ReinsertEntryInOrder(old_indices[i]);
  }

  for (std::size_t i = 0; i < first_ideal; ++i) {
    ReinsertEntryInOrder(old_indices[i]);
  }
}

void Map::ReserveOne() {
  const auto len = entries_.size();

  if (danger_.IsYellow()) {
    const auto load_factor =
        static_cast<float>(entries_.size()) / indices_.size();

    if (load_factor >= kLoadFactorThreshold) {
      danger_.ToGreen();

      const auto new_capacity = indices_.size() * 2;

      Grow(new_capacity);
    } else {
      danger_.ToRed();

      Rebuild();
    }
  } else if (len == Capacity()) {
    if (len == 0) {
      const auto new_raw_capacity = 8;
      mask_ = 7U;
      indices_ = std::vector<Pos>(new_raw_capacity, Pos::None());
      entries_.reserve(UsableCapacity(new_raw_capacity));
    } else {
      const auto raw_capacity = indices_.size();
      Grow(raw_capacity * 2);
    }
  }
}

void Map::ReinsertEntryInOrder(Pos pos) {
  if (pos.IsSome()) {
    const auto probe = DesiredPos(mask_, pos.hash);

    ProbeLoop(probe, [this, pos](std::size_t idx) {
      if (indices_[idx].IsNone()) {
        indices_[idx] = pos;
        return true;
      } else {
        return false;
      }
    });
  }
}

void Map::Rebuild() {}

std::size_t Map::Capacity() const noexcept {
  return UsableCapacity(indices_.size());
}

template <typename Fn>
std::size_t Map::ProbeLoop(std::size_t starting_position,
                           Fn&& probe_body) const {
  while (true) {
    if (starting_position < indices_.size()) {
      if (probe_body(starting_position)) {
        return starting_position;
      }
      ++starting_position;
    } else {
      starting_position = 0;
    }
  }
}

Traits::HashValue Map::HashKey(std::string_view key) const noexcept {
  constexpr std::size_t kMask = Traits::kMaxSize - 1;

  return static_cast<Traits::HashValue>(danger_.HashKey(key) & kMask);
}

void Map::InsertEntry(Traits::Key&& key, Traits::Value&& value) {
  entries_.emplace_back(std::move(key), std::move(value));
}

Map::Iterator Map::Find(std::string_view key) const noexcept {
  const auto pos = DoFind(key);

  return pos.IsSome() ? (entries_ref_.begin() + pos.index) : entries_ref_.end();
}

Pos Map::DoFind(std::string_view key) const noexcept {
  const auto hash = HashKey(key);
  const auto probe = DesiredPos(mask_, hash);

  std::size_t dist = 0;

  auto res = Pos::None();
  ProbeLoop(probe, [this, &key, hash, &dist, &res](std::size_t idx) {
    if (indices_[idx].IsNone()) {
      return true;
    }

    if (dist > ProbeDistance(mask_, hash, idx)) {
      return true;
    } else if (indices_[idx].hash == hash &&
               entries_[indices_[idx].index].header_name == key) {
      res = indices_[idx];
      return true;
    }

    ++dist;
    return false;
  });

  return res;
}

void Map::Insert(Traits::Key key, Traits::Value value) {
  ReserveOne();

  const auto hash = HashKey(key);
  const auto probe = DesiredPos(mask_, hash);
  std::size_t dist = 0;

  const auto inserter = [this, &key, &value, &dist, hash](std::size_t probe) {
    if (indices_[probe].IsSome()) {
      // The slot is already occupied, but check if it has a lower
      // displacement.
      const auto their_dist = ProbeDistance(mask_, indices_[probe].hash, probe);

      if (dist > their_dist) {
        // The new key's distance is larger, so claim this spot and
        // displace the current entry.
        //
        // Check if this insertion is above the danger threshold.
        const auto danger = dist >= kForwardShiftThreshold && !danger_.IsRed();

        // Robinhood
        {
          const auto index = entries_.size();
          InsertEntry(std::move(key), std::move(value));

          const auto num_displaced =
              DoRobinhoodAtPosition(probe, Pos{index, hash});

          if (danger || num_displaced >= kDisplacementThreshold) {
            danger_.ToYellow();
          }
        }

        return true;
      } else if (indices_[probe].hash == hash &&
                 // TODO : case-insensitive cmp
                 entries_[indices_[probe].index].header_name == key) {
        // TODO : do occupied, either append or replace

        return true;
      }
    } else {
      const auto danger = dist >= kForwardShiftThreshold && !danger_.IsRed();

      // TODO : do vacant properly
      const auto index = entries_.size();
      InsertEntry(std::move(key), std::move(value));
      indices_[probe] = Pos{index, hash};

      return true;
    }

    ++dist;
    return false;
  };

  ProbeLoop(probe, inserter);
}

std::size_t Map::DoRobinhoodAtPosition(std::size_t idx, Pos old_pos) {
  std::size_t num_displaced = 0;
  ProbeLoop(idx, [this, num_displaced, old_pos](std::size_t idx) mutable {
    auto& pos = indices_[idx];
    if (pos.IsNone()) {
      pos = old_pos;
      return true;
    } else {
      ++num_displaced;
      std::swap(pos, old_pos);
      return false;
    }
  });

  return num_displaced;
}

Map::Iterator Map::Begin() const noexcept { return entries_ref_.begin(); }

Map::Iterator Map::End() const noexcept { return entries_ref_.end(); }

}  // namespace server::http::header_map_impl

USERVER_NAMESPACE_END
