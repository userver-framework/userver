#pragma once

#include <string>
#include <unordered_map>
#include <unordered_set>
#include <vector>

#include <formats/json/value.hpp>
#include <storages/postgres/dsn.hpp>

namespace storages {
namespace postgres {
namespace secdist {

class PostgresSettings {
 public:
  explicit PostgresSettings(const formats::json::Value& doc);

  std::vector<DSNList> GetShardedClusterDescription(
      const std::string& dbalias) const;

 private:
  std::unordered_map<std::string, std::vector<DSNList>> sharded_cluster_descs_;
  std::unordered_set<std::string> invalid_dbaliases_;
};

}  // namespace secdist
}  // namespace postgres
}  // namespace storages
