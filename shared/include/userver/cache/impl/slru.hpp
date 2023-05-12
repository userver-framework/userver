#pragma once

#include <memory>
#include <utility>

#include <userver/cache/impl/lru.hpp>

/*

Hit rate on real data:

Keys count    Cache size        Lru       Slru
1988740             1000  0.0561333  0.0502915
                    5000  0.0860699  0.0816721
                   10000   0.110102   0.111687
                   20000   0.151564   0.158751
                   50000   0.274651   0.275751
                  100000   0.428465   0.424846

3595824             1000   0.357814   0.377788
                    5000   0.697188   0.705449
                   10000   0.831651    0.83536
                   20000   0.920158   0.920761
                   50000   0.971804   0.971791
                  100000   0.977242   0.977242

*/

USERVER_NAMESPACE_BEGIN

namespace cache::impl {

template <typename T, typename U, typename Hash = std::hash<T>,
          typename Equal = std::equal_to<T>>
class SlruBase final {
 public:
  using NodeType = std::unique_ptr<LruNode<T, U>>;

  explicit SlruBase(std::size_t probation_size, std::size_t protected_size,
                    const Hash& hash = Hash(), const Equal& equal = Equal());
  ~SlruBase() = default;

  SlruBase(SlruBase&& other) noexcept = default;

  SlruBase& operator=(SlruBase&& other) noexcept = default;

  SlruBase(const SlruBase& slru) = delete;
  SlruBase& operator=(const SlruBase& slru) = delete;

  bool Put(const T& key, U value);

  template <typename... Args>
  U* Emplace(const T& key, Args&&... args);

  void Erase(const T& key);

  U* Get(const T& key);

  const T* GetLeastUsedKey() const;

  U* GetLeastUsedValue();

  NodeType ExtractLeastUsedNode();

  void SetMaxSize(std::size_t new_probation_size,
                  std::size_t new_protected_size);

  void Clear() noexcept;

  template <typename Function>
  void VisitAll(Function&& func) const;

  template <typename Function>
  void VisitAll(Function&& func);

  std::size_t GetSize() const;

  std::size_t GetCapacity() const;

  U& InsertNode(NodeType&& node) noexcept;
  NodeType ExtractNode(const T& key) noexcept;

 private:
  cache::impl::LruBase<T, U, Hash, Equal> probation_part_;
  cache::impl::LruBase<T, U, Hash, Equal> protected_part_;
};

template <typename T, typename U, typename Hash, typename Equal>
SlruBase<T, U, Hash, Equal>::SlruBase(std::size_t probation_size,
                                      std::size_t protected_size,
                                      const Hash& hash, const Equal& equal)
    : probation_part_(probation_size, hash, equal),
      protected_part_(protected_size, hash, equal) {}

template <typename T, typename U, typename Hash, typename Equal>
bool SlruBase<T, U, Hash, Equal>::Put(const T& key, U value) {
  auto* const value_ptr = protected_part_.Get(key);
  if (value_ptr) {
    *value_ptr = std::move(value);
    return false;
  }

  auto node_to_protected = probation_part_.ExtractNode(key);
  if (node_to_protected) {
    node_to_protected->SetValue(std::move(value));
    if (protected_part_.GetSize() == protected_part_.GetCapacity()) {
      auto node_to_probation = protected_part_.ExtractLeastUsedNode();
      probation_part_.InsertNode(std::move(node_to_probation));
    }
    protected_part_.InsertNode(std::move(node_to_protected));
    return false;
  }

  return probation_part_.Put(key, std::move(value));
}

template <typename T, typename U, typename Hash, typename Equal>
template <typename... Args>
U* SlruBase<T, U, Hash, Equal>::Emplace(const T& key, Args&&... args) {
  auto value_ptr = protected_part_.Get(key);
  if (value_ptr) {
    return value_ptr;
  }

  auto node_to_protected = probation_part_.ExtractNode(key);
  if (node_to_protected) {
    return protected_part_.Emplace(key, std::forward<Args>(args)...);
  }

  return probation_part_.Emplace(key, std::forward<Args>(args)...);
}

template <typename T, typename U, typename Hash, typename Equal>
void SlruBase<T, U, Hash, Equal>::Erase(const T& key) {
  probation_part_.Erase(key);
  protected_part_.Erase(key);
}

template <typename T, typename U, typename Hash, typename Equal>
U* SlruBase<T, U, Hash, Equal>::Get(const T& key) {
  auto value_ptr = protected_part_.Get(key);
  if (value_ptr) {
    return value_ptr;
  }

  auto node_to_protected = probation_part_.ExtractNode(key);
  if (node_to_protected) {
    if (protected_part_.GetSize() == protected_part_.GetCapacity()) {
      auto node_to_probation = protected_part_.ExtractLeastUsedNode();
      probation_part_.InsertNode(std::move(node_to_probation));
    }
    return &protected_part_.InsertNode(std::move(node_to_protected));
  }
  return nullptr;
}

template <typename T, typename U, typename Hash, typename Equal>
const T* SlruBase<T, U, Hash, Equal>::GetLeastUsedKey() const {
  return probation_part_.GetLeastUsedKey();
}

template <typename T, typename U, typename Hash, typename Equal>
U* SlruBase<T, U, Hash, Equal>::GetLeastUsedValue() {
  return probation_part_.GetLeastUsedValue();
}

template <typename T, typename U, typename Hash, typename Equal>
typename SlruBase<T, U, Hash, Equal>::NodeType
SlruBase<T, U, Hash, Equal>::ExtractLeastUsedNode() {
  return probation_part_.ExtractLeastUsedNode();
}

template <typename T, typename U, typename Hash, typename Equal>
void SlruBase<T, U, Hash, Equal>::SetMaxSize(std::size_t new_probation_size,
                                             std::size_t new_protected_size) {
  probation_part_.SetMaxSize(new_probation_size);
  protected_part_.SetMaxSize(new_protected_size);
}

template <typename T, typename U, typename Hash, typename Equal>
void SlruBase<T, U, Hash, Equal>::Clear() noexcept {
  probation_part_.Clear();
  protected_part_.Clear();
}

template <typename T, typename U, typename Hash, typename Equal>
template <typename Function>
void SlruBase<T, U, Hash, Equal>::VisitAll(Function&& func) const {
  probation_part_.VisitAll(std::forward<Function>(func));
  protected_part_.VisitAll(std::forward<Function>(func));
}

template <typename T, typename U, typename Hash, typename Equal>
template <typename Function>
void SlruBase<T, U, Hash, Equal>::VisitAll(Function&& func) {
  probation_part_.VisitAll(std::forward<Function>(func));
  protected_part_.VisitAll(std::forward<Function>(func));
}

template <typename T, typename U, typename Hash, typename Equal>
std::size_t SlruBase<T, U, Hash, Equal>::GetSize() const {
  return probation_part_.GetSize() + protected_part_.GetSize();
}

template <typename T, typename U, typename Hash, typename Equal>
std::size_t SlruBase<T, U, Hash, Equal>::GetCapacity() const {
  return probation_part_.GetCapacity() + protected_part_.GetCapacity();
}

template <typename T, typename U, typename Hash, typename Equal>
U& SlruBase<T, U, Hash, Equal>::InsertNode(NodeType&& node) noexcept {
  return probation_part_.InsertNode(std::move(node));
}

template <typename T, typename U, typename Hash, typename Equal>
typename SlruBase<T, U, Hash, Equal>::NodeType
SlruBase<T, U, Hash, Equal>::ExtractNode(const T& key) noexcept {
  auto protected_node = protected_part_.ExtractNode(key);
  if (protected_node) {
    return protected_node;
  }

  return probation_part_.ExtractNode(key);
}

}  // namespace cache::impl

USERVER_NAMESPACE_END
