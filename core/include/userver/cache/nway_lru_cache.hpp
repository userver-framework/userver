#pragma once

#include <functional>
#include <optional>
#include <vector>

#include <boost/container_hash/hash.hpp>

#include <userver/cache/lru_map.hpp>
#include <userver/dump/dumper.hpp>
#include <userver/dump/operations.hpp>
#include <userver/engine/mutex.hpp>

USERVER_NAMESPACE_BEGIN

namespace cache {

/// @ingroup userver_containers
template <typename T, typename U, typename Hash = std::hash<T>,
          typename Equal = std::equal_to<T>>
class NWayLRU final {
 public:
  /// @param ways is the number of ways (a.k.a. shards, internal hash-maps),
  /// into which elements are distributed based on their hash. Each shard is
  /// protected by an individual mutex. Larger `ways` means more internal
  /// hash-map instances and more memory usage, but less contention. A good
  /// starting point is `ways=16`. If you encounter contention, you can increase
  /// `ways` to something on the order of `256` or whatever your RAM constraints
  /// allow.
  ///
  /// @param way_size is the maximum allowed amount of elements per way. When
  /// the size of a way reaches this number, existing elements are deleted
  /// according to the LRU policy.
  ///
  /// The maximum total number of elements is `ways * way_size`.
  NWayLRU(size_t ways, size_t way_size, const Hash& hash = Hash(),
          const Equal& equal = Equal());

  void Put(const T& key, U value);

  template <typename Validator>
  std::optional<U> Get(const T& key, Validator validator);

  std::optional<U> Get(const T& key) {
    return Get(key, [](const U&) { return true; });
  }

  U GetOr(const T& key, const U& default_value);

  void Invalidate();

  void InvalidateByKey(const T& key);

  /// Iterates over all items. May be slow for big caches.
  template <typename Function>
  void VisitAll(Function func) const;

  size_t GetSize() const;

  /// For the description of `way_size`,
  /// see the cache::NWayLRU::NWayLRU constructor.
  void UpdateWaySize(size_t way_size);

  void Write(dump::Writer& writer) const;
  void Read(dump::Reader& reader);

  /// The dump::Dumper will be notified of any cache updates. This method is not
  /// thread-safe.
  void SetDumper(std::shared_ptr<dump::Dumper> dumper);

 private:
  struct Way {
    Way(Way&& other) noexcept : cache(std::move(other.cache)) {}

    // max_size is not used, will be reset by Resize() in NWayLRU::NWayLRU
    Way(const Hash& hash, const Equal& equal) : cache(1, hash, equal) {}

    mutable engine::Mutex mutex;
    LruMap<T, U, Hash, Equal> cache;
  };

  Way& GetWay(const T& key);

  void NotifyDumper();

  std::vector<Way> caches_;
  Hash hash_fn_;
  std::shared_ptr<dump::Dumper> dumper_{nullptr};
};

template <typename T, typename U, typename Hash, typename Eq>
NWayLRU<T, U, Hash, Eq>::NWayLRU(size_t ways, size_t way_size, const Hash& hash,
                                 const Eq& equal)
    : caches_(), hash_fn_(hash) {
  caches_.reserve(ways);
  for (size_t i = 0; i < ways; ++i) caches_.emplace_back(hash, equal);
  if (ways == 0) throw std::logic_error("Ways must be positive");

  for (auto& way : caches_) way.cache.SetMaxSize(way_size);
}

template <typename T, typename U, typename Hash, typename Eq>
void NWayLRU<T, U, Hash, Eq>::Put(const T& key, U value) {
  auto& way = GetWay(key);
  {
    std::unique_lock<engine::Mutex> lock(way.mutex);
    way.cache.Put(key, std::move(value));
  }
  NotifyDumper();
}

template <typename T, typename U, typename Hash, typename Eq>
template <typename Validator>
std::optional<U> NWayLRU<T, U, Hash, Eq>::Get(const T& key,
                                              Validator validator) {
  auto& way = GetWay(key);
  std::unique_lock<engine::Mutex> lock(way.mutex);
  auto* value = way.cache.Get(key);

  if (value) {
    if (validator(*value)) return *value;
    way.cache.Erase(key);
  }

  return std::nullopt;
}

template <typename T, typename U, typename Hash, typename Eq>
void NWayLRU<T, U, Hash, Eq>::InvalidateByKey(const T& key) {
  auto& way = GetWay(key);
  {
    std::unique_lock<engine::Mutex> lock(way.mutex);
    way.cache.Erase(key);
  }
  NotifyDumper();
}

template <typename T, typename U, typename Hash, typename Eq>
U NWayLRU<T, U, Hash, Eq>::GetOr(const T& key, const U& default_value) {
  auto& way = GetWay(key);
  std::unique_lock<engine::Mutex> lock(way.mutex);
  return way.cache.GetOr(key, default_value);
}

template <typename T, typename U, typename Hash, typename Eq>
void NWayLRU<T, U, Hash, Eq>::Invalidate() {
  for (auto& way : caches_) {
    std::unique_lock<engine::Mutex> lock(way.mutex);
    way.cache.Clear();
  }
  NotifyDumper();
}

template <typename T, typename U, typename Hash, typename Eq>
template <typename Function>
void NWayLRU<T, U, Hash, Eq>::VisitAll(Function func) const {
  for (const auto& way : caches_) {
    std::unique_lock<engine::Mutex> lock(way.mutex);
    way.cache.VisitAll(func);
  }
}

template <typename T, typename U, typename Hash, typename Eq>
size_t NWayLRU<T, U, Hash, Eq>::GetSize() const {
  size_t size{0};
  for (const auto& way : caches_) {
    std::unique_lock<engine::Mutex> lock(way.mutex);
    size += way.cache.GetSize();
  }
  return size;
}

template <typename T, typename U, typename Hash, typename Eq>
void NWayLRU<T, U, Hash, Eq>::UpdateWaySize(size_t way_size) {
  for (auto& way : caches_) {
    std::unique_lock<engine::Mutex> lock(way.mutex);
    way.cache.SetMaxSize(way_size);
  }
}

template <typename T, typename U, typename Hash, typename Eq>
typename NWayLRU<T, U, Hash, Eq>::Way& NWayLRU<T, U, Hash, Eq>::GetWay(
    const T& key) {
  /// It is needed to twist hash because there is hash map in LruMap. Otherwise
  /// nodes will fall into one bucket. According to
  /// https://www.boost.org/doc/libs/1_83_0/libs/container_hash/doc/html/hash.html#notes_hash_combine
  /// hash_combine can be treated as hash itself
  auto seed = hash_fn_(key);
  boost::hash_combine(seed, 0);
  auto n = seed % caches_.size();
  return caches_[n];
}

template <typename T, typename U, typename Hash, typename Equal>
void NWayLRU<T, U, Hash, Equal>::Write(dump::Writer& writer) const {
  writer.Write(caches_.size());

  for (const Way& way : caches_) {
    std::unique_lock<engine::Mutex> lock(way.mutex);

    writer.Write(way.cache.GetSize());

    way.cache.VisitAll([&writer](const T& key, const U& value) {
      writer.Write(key);
      writer.Write(value);
    });
  }
}

template <typename T, typename U, typename Hash, typename Equal>
void NWayLRU<T, U, Hash, Equal>::Read(dump::Reader& reader) {
  Invalidate();

  const auto ways = reader.Read<std::size_t>();
  for (std::size_t i = 0; i < ways; ++i) {
    const auto elements_in_way = reader.Read<std::size_t>();
    for (std::size_t j = 0; j < elements_in_way; ++j) {
      auto key = reader.Read<T>();
      auto value = reader.Read<U>();
      Put(std::move(key), std::move(value));
    }
  }
}

template <typename T, typename U, typename Hash, typename Equal>
void NWayLRU<T, U, Hash, Equal>::NotifyDumper() {
  if (dumper_ != nullptr) {
    dumper_->OnUpdateCompleted();
  }
}

template <typename T, typename U, typename Hash, typename Equal>
void NWayLRU<T, U, Hash, Equal>::SetDumper(
    std::shared_ptr<dump::Dumper> dumper) {
  dumper_ = std::move(dumper);
}

}  // namespace cache

USERVER_NAMESPACE_END
