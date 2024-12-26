#include <userver/storages/postgres/cluster.hpp>
#include <userver/storages/postgres/component.hpp>
#include <userver/storages/postgres/exceptions.hpp>

USERVER_NAMESPACE_BEGIN

namespace storages::postgres {

ClusterPtr Database::GetCluster() const { return GetClusterForShard(components::Postgres::kDefaultShardNumber); }

ClusterPtr Database::GetClusterForShard(size_t shard) const {
    if (shard >= GetShardCount()) {
        throw storages::postgres::ClusterUnavailable("Shard number " + std::to_string(shard) + " is out of range");
    }

    return clusters_[shard];
}

void Database::UpdateClusterDescription(const std::vector<DsnList>& dsns) {
    if (dsns.size() != GetShardCount()) {
        throw std::runtime_error(fmt::format(
            "Invalid PG secdist configuration: was initialized with {} shards, now update with {} shards",
            GetShardCount(),
            dsns.size()
        ));
    }

    auto dsn_it = dsns.begin();
    for (auto& cluster : clusters_) {
        cluster->SetDsnList(*dsn_it++);
    }
}

}  // namespace storages::postgres

USERVER_NAMESPACE_END
