#include <storages/postgres/postgres_secdist.hpp>

#include <set>

#include <storages/secdist/exceptions.hpp>
#include <storages/secdist/helpers.hpp>

#include <logging/log.hpp>

namespace storages {
namespace postgres {
namespace secdist {

namespace {

void CheckSingleHostDSN(const std::string& dsn) {
  auto dsns = storages::postgres::SplitByHost(dsn);
  if (dsns.empty()) {
    throw storages::secdist::SecdistError(
        DsnMaskPassword(dsn) + " doesn't seem as a valid PostgreSQL DSN");
  }
  if (dsns.size() > 1) {
    throw storages::secdist::SecdistError(
        DsnCutPassword(dsn) +
        " contains multiple hosts, this is not supported in this configuration "
        "type");
  }
}

ClusterDescription ClusterDescriptionFromJson(
    const formats::json::Value& elem) {
  if (elem.HasMember("hosts")) {
    auto hosts = elem["hosts"];
    storages::secdist::CheckIsArray(hosts, "hosts");
    std::set<std::string> dsn_set;
    for (auto host_it = hosts.begin(); host_it != hosts.end(); ++host_it) {
      if (!host_it->IsString()) {
        storages::secdist::ThrowInvalidSecdistType(
            "hosts[" + std::to_string(host_it.GetIndex()) + ']', "a string");
      }
      auto dsn = host_it->As<std::string>();
      auto multihost = storages::postgres::SplitByHost(dsn);
      if (multihost.empty()) {
        throw storages::secdist::SecdistError(
            DsnMaskPassword(dsn) + " doesn't seem as a valid PostgreSQL DSN");
      }
      dsn_set.insert(multihost.begin(), multihost.end());
    }
    DSNList dsns{dsn_set.begin(), dsn_set.end()};
    return ClusterDescription{dsns};
  } else {
    auto master_dsn = storages::secdist::GetString(elem, "master");
    CheckSingleHostDSN(master_dsn);
    auto sync_slave_dsn = storages::secdist::GetString(elem, "sync_slave");
    CheckSingleHostDSN(sync_slave_dsn);

    DSNList slave_dsns;
    const formats::json::Value& slaves = elem["slaves"];
    storages::secdist::CheckIsArray(slaves, "slaves");
    for (auto it = slaves.begin(); it != slaves.end(); ++it) {
      if (!it->IsString()) {
        storages::secdist::ThrowInvalidSecdistType(
            "slaves[" + std::to_string(it.GetIndex()) + ']', "a string");
      }
      auto dsn = it->As<std::string>();
      CheckSingleHostDSN(dsn);
      slave_dsns.push_back(dsn);
    }

    return ClusterDescription{master_dsn, sync_slave_dsn, slave_dsns};
  }
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

    auto& sharded_cluster_for_db = sharded_cluster_descs_[dbalias];
    sharded_cluster_for_db.reserve(cluster_array.GetSize());

    size_t shard_num_expect = 0;
    try {
      for (auto shard_it = cluster_array.begin();
           shard_it != cluster_array.end(); ++shard_it) {
        storages::secdist::CheckIsObject(
            *shard_it,
            dbalias + '[' + std::to_string(shard_it.GetIndex() + ']'));
        // Legacy check, expect shard_num to be in order
        const auto shard_num = (*shard_it)["shard_number"].As<uint64_t>();
        if (shard_num != shard_num_expect) {
          throw storages::secdist::SecdistError(
              "shard_number " + std::to_string(shard_num) +
              " is out of order. Should equal to " +
              std::to_string(shard_num_expect));
        }
        ++shard_num_expect;
        sharded_cluster_for_db.push_back(ClusterDescriptionFromJson(*shard_it));
      }
    } catch (storages::secdist::SecdistError& e) {
      LOG_WARNING() << "Secdist for " << dbalias
                    << " contains unsupported formats: " << e;
      sharded_cluster_descs_.erase(dbalias);
      invalid_dbaliases_.insert(dbalias);
    }
  }
}

ShardedClusterDescription PostgresSettings::GetShardedClusterDescription(
    const std::string& dbalias) const {
  auto it = sharded_cluster_descs_.find(dbalias);

  if (it == sharded_cluster_descs_.end()) {
    if (invalid_dbaliases_.count(dbalias)) {
      throw storages::secdist::SecdistError(
          "dbalias " + dbalias + " secdist config is in unsupported format");
    }
    throw storages::secdist::UnknownPostgresDbAlias(
        "dbalias " + dbalias + " not found in secdist config");
  }
  return it->second;
}

}  // namespace secdist
}  // namespace postgres
}  // namespace storages
