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

  for (auto it = databases.begin(); it != databases.end(); ++it) {
    const std::string& dbalias = it.GetName();
    const formats::json::Value& cluster_array = *it;
    storages::secdist::CheckIsArray(cluster_array, dbalias);

    // TODO: read the whole array not just 0 element
    storages::secdist::CheckIsObject(cluster_array[0], dbalias + "[0]");
    cluster_descs_[dbalias] = ClusterDescriptionFromJson(cluster_array[0]);
  }
}

const ClusterDescription& PostgresSettings::GetClusterDescription(
    const std::string& dbalias) const {
  auto it = cluster_descs_.find(dbalias);

  if (it == cluster_descs_.end())
    throw storages::secdist::UnknownPostgresDbAlias(
        "dbalias " + dbalias + " not found in secdist config");

  return it->second;
}

}  // namespace secdist
}  // namespace postgres
}  // namespace storages
