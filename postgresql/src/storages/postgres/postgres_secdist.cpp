#include <storages/postgres/postgres_secdist.hpp>

#include <ranges>
#include <set>

#include <fmt/format.h>
#include <fmt/ranges.h>

#include <userver/formats/common/items.hpp>
#include <userver/logging/log.hpp>
#include <userver/storages/secdist/exceptions.hpp>
#include <userver/storages/secdist/helpers.hpp>
#include <userver/utils/enumerate.hpp>

USERVER_NAMESPACE_BEGIN

namespace storages::postgres::secdist {

namespace {

DsnList DsnListFromJson(const formats::json::Value& elem) {
    auto hosts = elem["hosts"];
    storages::secdist::CheckIsArray(hosts, "hosts");
    std::set<Dsn> dsn_set;
    for (const auto& host : hosts) {
        if (!host.IsString()) {
            storages::secdist::ThrowInvalidSecdistType(host, "a string");
        }
        const Dsn dsn{host.As<std::string>()};
        auto multihost = storages::postgres::SplitByHost(dsn);
        if (multihost.empty()) {
            throw storages::secdist::SecdistError(DsnMaskPassword(dsn) + " doesn't seem as a valid PostgreSQL Dsn");
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

    for (const auto& [dbalias, cluster_array] : formats::common::Items(databases)) {
        storages::secdist::CheckIsArray(cluster_array, dbalias);

        auto& sharded_cluster_for_db = sharded_cluster_descs_[dbalias];
        sharded_cluster_for_db.reserve(cluster_array.GetSize());

        try {
            for (auto [shard_index, shard] : utils::enumerate(cluster_array)) {
                storages::secdist::CheckIsObject(shard, dbalias + '[' + std::to_string(shard_index) + ']');
                // Legacy check, expect shard_num to be in order
                const auto shard_num = shard["shard_number"].As<uint64_t>();
                if (shard_num != shard_index) {
                    throw storages::secdist::SecdistError(
                        "shard_number " + std::to_string(shard_num) + " is out of order. Should equal to " +
                        std::to_string(shard_index)
                    );
                }
                sharded_cluster_for_db.push_back(DsnListFromJson(shard));
            }
        } catch (storages::secdist::SecdistError& e) {
            LOG_WARNING() << "Secdist for " << dbalias << " contains unsupported formats: " << e;
            sharded_cluster_descs_.erase(dbalias);
            invalid_dbaliases_.insert(dbalias);
        }
    }
}

std::vector<DsnList> PostgresSettings::GetShardedClusterDescription(const std::string& dbalias) const {
    auto it = sharded_cluster_descs_.find(dbalias);

    if (it == sharded_cluster_descs_.end()) {
        if (invalid_dbaliases_.count(dbalias)) {
            throw storages::secdist::SecdistError("dbalias " + dbalias + " secdist config is in unsupported format");
        }
        throw storages::secdist::UnknownPostgresDbAlias(fmt::format(
            "dbalias {} not found in secdist config. Available aliases: [{}]",
            dbalias,
            fmt::join(sharded_cluster_descs_ | std::views::keys, ", ")
        ));
    }
    return it->second;
}

}  // namespace storages::postgres::secdist

USERVER_NAMESPACE_END
