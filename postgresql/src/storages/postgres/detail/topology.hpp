#pragma once

#include <memory>
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
  using HostsByType =
      std::unordered_map<ClusterHostType, DSNList, ClusterHostTypeHash>;

 public:
  virtual ~ClusterTopology() = default;

  const DSNList& GetDsnList() const { return dsn_list_; }
  virtual HostsByType GetHostsByType() const = 0;

 protected:
  DSNList dsn_list_;
};

using ClusterTopologyPtr = std::unique_ptr<ClusterTopology>;

}  // namespace detail
}  // namespace postgres
}  // namespace storages
