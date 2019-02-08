#pragma once

#include <string>
#include <unordered_map>
#include <unordered_set>

#include <formats/json/value.hpp>

#include <storages/postgres/cluster_types.hpp>

namespace storages {
namespace postgres {
namespace secdist {

class PostgresSettings {
 public:
  explicit PostgresSettings(const formats::json::Value& doc);

  ShardedClusterDescription GetShardedClusterDescription(
      const std::string& dbalias) const;

 private:
  std::unordered_map<std::string, ShardedClusterDescription>
      sharded_cluster_descs_;
  std::unordered_set<std::string> invalid_dbaliases_;
};

}  // namespace secdist
}  // namespace postgres
}  // namespace storages
