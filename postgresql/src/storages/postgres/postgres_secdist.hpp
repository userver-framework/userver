#pragma once

#include <string>
#include <unordered_map>
#include <unordered_set>
#include <vector>

#include <userver/formats/json/value.hpp>
#include <userver/storages/postgres/dsn.hpp>

USERVER_NAMESPACE_BEGIN

namespace storages::postgres::secdist {

class PostgresSettings {
 public:
  explicit PostgresSettings(const formats::json::Value& doc);

  std::vector<DsnList> GetShardedClusterDescription(
      const std::string& dbalias) const;

 private:
  std::unordered_map<std::string, std::vector<DsnList>> sharded_cluster_descs_;
  std::unordered_set<std::string> invalid_dbaliases_;
};

}  // namespace storages::postgres::secdist

USERVER_NAMESPACE_END
