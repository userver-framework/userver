#pragma once

#include <atomic>
#include <cstddef>
#include <cstdint>
#include <optional>
#include <span>

#include <storages/postgres/detail/rtt.hpp>
#include <storages/postgres/detail/topology/base.hpp>
#include <userver/storages/postgres/cluster_types.hpp>

USERVER_NAMESPACE_BEGIN

namespace storages::postgres::detail {

topology::TopologyBase::DsnIndicesByType::const_iterator ResolveHostRole(
    const topology::TopologyBase::DsnIndicesByType& dsn_indices_by_type,
    ClusterHostType requested_role
);

std::size_t SelectDsnIndex(
    const topology::TopologyBase::DsnIndices& dsn_indices,
    ClusterHostTypeFlags flags,
    std::atomic<std::uint32_t>& rr_host_idx
);

void FillDsnIndices(
    topology::TopologyBase::DsnIndices& dsn_indices,
    std::span<const Rtt> roundtrip_times,
    std::optional<Rtt> max_rtt
);

}  // namespace storages::postgres::detail

USERVER_NAMESPACE_END
