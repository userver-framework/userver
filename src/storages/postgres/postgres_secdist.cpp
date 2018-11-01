#include <storages/postgres/postgres_secdist.hpp>

#include <storages/secdist/exceptions.hpp>
#include <storages/secdist/helpers.hpp>

namespace storages {
namespace postgres {
namespace secdist {

namespace {

ClusterDescription ClusterDescriptionFromJson(
    const formats::json::Value& elem) {
  auto master_dsn = storages::secdist::GetString(elem, "master");
  auto sync_slave_dsn = storages::secdist::GetString(elem, "sync_slave");

  DSNList slave_dsns;
  const formats::json::Value& slaves = elem["slaves"];
  storages::secdist::CheckIsArray(slaves, "slaves");
  for (auto it = slaves.begin(); it != slaves.end(); ++it) {
    if (!it->isString()) {
      storages::secdist::ThrowInvalidSecdistType(
          "slaves[" + std::to_string(it.GetIndex()) + ']', "a string");
    }
    slave_dsns.push_back(it->asString());
  }

  return {master_dsn, sync_slave_dsn, slave_dsns};
}

}  // namespace

PostgresSettings::PostgresSettings(const formats::json::Value& doc) {
  const formats::json::Value& postgresql_settings = doc["postgresql_settings"];
  storages::secdist::CheckIsObject(postgresql_settings, "postgresql_settings");

  const formats::json::Value& databases = postgresql_settings["databases"];
  storages::secdist::CheckIsObject(databases, "databases");

  for (auto db_it = databases.begin(); db_it != databases.end(); ++db_it) {
    const std::string& dbalias = db_it.GetName();
    const formats::json::Value& cluster_array = *db_it;
    storages::secdist::CheckIsArray(cluster_array, dbalias);

    for (auto shard_it = cluster_array.begin(); shard_it != cluster_array.end();
         ++shard_it) {
      storages::secdist::CheckIsObject(
          *shard_it, dbalias + '[' + std::to_string(shard_it.GetIndex() + ']'));
      const auto shard_num = (*shard_it)["shard_number"].asUInt64();
      sharded_cluster_descs_[dbalias][shard_num] =
          ClusterDescriptionFromJson(*shard_it);
    }
  }
}

ShardedClusterDescription PostgresSettings::GetShardedClusterDescription(
    const std::string& dbalias) const {
  auto it = sharded_cluster_descs_.find(dbalias);

  if (it == sharded_cluster_descs_.end()) {
    throw storages::secdist::UnknownPostgresDbAlias(
        "dbalias " + dbalias + " not found in secdist config");
  }
  return it->second;
}

}  // namespace secdist
}  // namespace postgres
}  // namespace storages
