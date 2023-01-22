#include <userver/server/http/header_map.hpp>

// Shameless adaptation of
// https://github.com/hyperium/http/blob/f0ba97fe20054d6ef83834f92a0842be947996bd/src/header/map.rs

#include <limits>
#include <optional>
#include <string>
#include <vector>

#include <userver/utils/assert.hpp>
#include <userver/utils/rand.hpp>

USERVER_NAMESPACE_BEGIN

namespace server::http {

using Size = std::uint16_t;
using HashValue = Size;

constexpr std::size_t kMaxSize = 1 << 15;
static_assert(kMaxSize < std::numeric_limits<Size>::max());

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

struct Pos final {
  // Index in the 'entries' vector
  Size index;
  // Full hash value for the entry
  HashValue hash;

  Pos() = default;

  Pos(std::size_t index, HashValue hash) : Pos{static_cast<Size>(index), hash} {
    UASSERT(index < kMaxSize);
  }

  Pos(Size index, HashValue hash) : index{index}, hash{hash} {}

  static Pos None() { return Pos{std::numeric_limits<Size>::max(), 0}; }

  bool IsNone() const noexcept {
    return index == std::numeric_limits<Size>::max();
  }

  bool IsSome() const noexcept { return !IsNone(); }
};

using Key = std::string;
using Value = std::string;

class Danger final {
 public:
  std::size_t HashKey(const Key& key) const noexcept {
    if (!IsRed()) {
      return UnsafeHash(key);
    }

    return SafeHash(hash_seed_, key);
  }

  bool IsGreen() const noexcept { return state_ == State::kGreen; }
  bool IsYellow() const noexcept { return state_ == State::kYellow; }
  bool IsRed() const noexcept { return state_ == State::kRed; }

  void ToGreen() noexcept {
    UASSERT(!IsRed());

    state_ = State::kGreen;
  }

  void ToYellow() noexcept {
    UASSERT(!IsRed());

    state_ = State::kYellow;
  }

  void ToRed() noexcept {
    state_ = State::kRed;

    do {
      hash_seed_ =
          std::uniform_int_distribution<std::size_t>{}(utils::DefaultRandom());
    } while (hash_seed_ == 0);
  }

 private:
  static std::size_t SafeHash(std::size_t seed, const Key& key) noexcept;
  static std::size_t UnsafeHash(const Key& key) noexcept;

  enum class State { kGreen, kYellow, kRed };

  State state_{State::kGreen};
  std::size_t hash_seed_{0};
};

struct Entry final {
  Key key;
  Value value;
};

class HeaderMap::Impl final {
 public:
  Impl() : mask_{0} {}

  std::size_t GetSize() const noexcept { return entries_.size(); }

  bool Empty() const noexcept { return entries_.empty(); }

  bool Find(const Key& key) const noexcept;

  void Insert(Key key, Value value);

 private:
  void ReserveOne();

  void Grow(std::size_t new_capacity);
  void ReinsertEntryInOrder(Pos pos);

  void Rebuild();

  std::size_t Capacity() const noexcept;

  template <typename Fn>
  std::size_t ProbeLoop(std::size_t starting_position, Fn&& probe_body) const;

  HashValue HashKey(const Key& key) const noexcept;

  void InsertEntry(Key&& key, Value&& value);

  Size mask_;
  std::vector<Pos> indices_;
  std::vector<Entry> entries_;
  Danger danger_;
};

std::size_t DesiredPos(Size mask, HashValue hash) noexcept {
  return static_cast<std::size_t>(hash & mask);
}

std::size_t ProbeDistance(Size mask, HashValue hash,
                          std::size_t current) noexcept {
  return static_cast<std::size_t>((current - DesiredPos(mask, hash)) & mask);
}

std::size_t UsableCapacity(std::size_t capacity) noexcept {
  return capacity - capacity / 4;
}

std::size_t ToRawCapacity(std::size_t capacity) noexcept {
  return capacity + capacity / 3;
}

HeaderMap::HeaderMap() {}

HeaderMap::~HeaderMap() {}

void HeaderMap::Insert(std::string key, std::string value) {
  impl_->Insert(std::move(key), std::move(value));
}

bool HeaderMap::Find(const std::string& key) const noexcept {
  return impl_->Find(key);
}

void HeaderMap::Impl::Grow(std::size_t new_capacity) {
  UINVARIANT(new_capacity <= kMaxSize, "Requested capacity too large");

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
  mask_ = static_cast<Size>(new_capacity - 1);

  for (std::size_t i = first_ideal; i < old_indices.size(); ++i) {
    ReinsertEntryInOrder(old_indices[i]);
  }

  for (std::size_t i = 0; i < first_ideal; ++i) {
    ReinsertEntryInOrder(old_indices[i]);
  }
}

void HeaderMap::Impl::ReserveOne() {
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

void HeaderMap::Impl::ReinsertEntryInOrder(Pos pos) {
  if (pos.IsSome()) {
    const auto probe = DesiredPos(mask_, pos.hash);

    ProbeLoop(probe, [this, pos](std::size_t idx) mutable {
      if (indices_[idx].IsNone()) {
        indices_[idx] = pos;
        return true;
      } else {
        return false;
      }
    });
  }
}

void HeaderMap::Impl::Rebuild() {}

std::size_t HeaderMap::Impl::Capacity() const noexcept {
  return UsableCapacity(indices_.size());
}

template <typename Fn>
std::size_t HeaderMap::Impl::ProbeLoop(std::size_t starting_position,
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

HashValue HeaderMap::Impl::HashKey(const Key& key) const noexcept {
  constexpr std::size_t kMask = kMaxSize - 1;

  return static_cast<HashValue>(danger_.HashKey(key) & kMask);
}

void HeaderMap::Impl::InsertEntry(Key&& key, Value&& value) {
  entries_.push_back(Entry{std::move(key), std::move(value)});
}

bool HeaderMap::Impl::Find(const Key& key) const noexcept {
  const auto hash = HashKey(key);
  const auto probe = DesiredPos(mask_, hash);

  bool found = false;
  std::size_t dist = 0;
  ProbeLoop(probe, [this, &key, hash, &dist, &found](std::size_t idx) {
    if (indices_[idx].IsNone()) {
      return true;
    }

    if (dist > ProbeDistance(mask_, hash, idx)) {
      return true;
    } else if (indices_[idx].hash == hash &&
               // TODO : case-insensitive cmp
               entries_[indices_[idx].index].key == key) {
      found = true;
      return true;
    }

    ++dist;
    return false;
  });

  return found;
}

void HeaderMap::Impl::Insert(Key key, Value value) {
  ReserveOne();

  const auto hash = HashKey(key);
  const auto probe = DesiredPos(mask_, hash);
  std::size_t dist = 0;

  ProbeLoop(probe, [this, &key, hash, &value, &dist](std::size_t idx) mutable {
    if (indices_[idx].IsSome()) {
      // The slot is already occupied, but check if it has a lower
      // displacement.
      const auto their_dist = ProbeDistance(mask_, indices_[idx].hash, idx);

      if (dist > their_dist) {
        // The new key's distance is larger, so claim this spot and
        // displace the current entry.
        //
        // Check if this insertion is above the danger threshold.
        const auto danger = dist >= kForwardShiftThreshold && !danger_.IsRed();

        // TODO : do robinhood

        return true;
      } else if (indices_[idx].hash == hash &&
                 // TODO : case-insensitive cmp
                 entries_[indices_[idx].index].key == key) {
        // TODO : do occupied, either append or replace

        return true;
      }
    } else {
      const auto danger = dist >= kForwardShiftThreshold && !danger_.IsRed();

      // TODO : do vacant properly
      const auto index = entries_.size();
      InsertEntry(std::move(key), std::move(value));
      indices_[idx] = Pos{index, hash};

      return true;
    }

    ++dist;
    return false;
  });
}

std::size_t Danger::SafeHash(std::size_t seed, const Key& key) noexcept {
  return 0;
}

std::size_t Danger::UnsafeHash(const Key& key) noexcept { return 0; }

}  // namespace server::http

USERVER_NAMESPACE_END
