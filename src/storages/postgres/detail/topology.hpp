#pragma once

#include <unordered_map>
#include <vector>

#include <storages/postgres/cluster_types.hpp>
#include <storages/postgres/dsn.hpp>

namespace storages {
namespace postgres {
namespace detail {

struct ClusterHostTypeHash {
  size_t operator()(const ClusterHostType& ht) const noexcept {
    return static_cast<size_t>(ht);
  }
};

class ClusterTopology {
 public:
  using HostIndices = std::vector<size_t>;

 public:
  explicit ClusterTopology(const ClusterDescription& cluster_desc);

  const DSNList& GetDsnList() const;
  HostIndices GetHostsByType(ClusterHostType ht) const;

 private:
  void AddHost(ClusterHostType host_type, const std::string& host);

 private:
  DSNList dsn_list_;
  std::unordered_map<ClusterHostType, HostIndices, ClusterHostTypeHash>
      host_to_indices_;
};

}  // namespace detail
}  // namespace postgres
}  // namespace storages
