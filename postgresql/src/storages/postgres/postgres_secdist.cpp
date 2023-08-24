#include <storages/postgres/postgres_secdist.hpp>

#include <set>

#include <userver/storages/secdist/exceptions.hpp>
#include <userver/storages/secdist/helpers.hpp>

#include <userver/logging/log.hpp>

USERVER_NAMESPACE_BEGIN

namespace storages::postgres::secdist {

namespace {

DsnList DsnListFromJson(const formats::json::Value& elem) {
  auto hosts = elem["hosts"];
  storages::secdist::CheckIsArray(hosts, "hosts");
  std::set<Dsn> dsn_set;
  for (auto host_it = hosts.begin(); host_it != hosts.end(); ++host_it) {
    if (!host_it->IsString()) {
      storages::secdist::ThrowInvalidSecdistType(*host_it, "a string");
    }
    Dsn dsn{host_it->As<std::string>()};
    auto multihost = storages::postgres::SplitByHost(dsn);
    if (multihost.empty()) {
      throw storages::secdist::SecdistError(
          DsnMaskPassword(dsn) + " doesn't seem as a valid PostgreSQL Dsn");
    }
    dsn_set.insert(multihost.begin(), multihost.end());
  }
  return {dsn_set.begin(), dsn_set.end()};
}

}  // namespace

PostgresSettings::PostgresSettings(const formats::json::Value& doc) {
  const formats::json::Value& postgresql_settings = doc["postgresql_settings"];
  if (postgresql_settings.IsMissing()) {
    LOG_WARNING() << "'postgresql_settings' secdist section is empty";
    return;
  }

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
        sharded_cluster_for_db.push_back(DsnListFromJson(*shard_it));
      }
    } catch (storages::secdist::SecdistError& e) {
      LOG_WARNING() << "Secdist for " << dbalias
                    << " contains unsupported formats: " << e;
      sharded_cluster_descs_.erase(dbalias);
      invalid_dbaliases_.insert(dbalias);
    }
  }
}

std::vector<DsnList> PostgresSettings::GetShardedClusterDescription(
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

}  // namespace storages::postgres::secdist

USERVER_NAMESPACE_END
