#include <storages/postgres/detail/host_selection.hpp>

#include <optional>

#include <fmt/format.h>

#include <userver/logging/log.hpp>
#include <userver/storages/postgres/exceptions.hpp>
#include <userver/utils/assert.hpp>

USERVER_NAMESPACE_BEGIN

namespace storages::postgres::detail {
namespace {

ClusterHostType Fallback(ClusterHostType host_type) {
    switch (host_type) {
        case ClusterHostType::kMaster:
            throw ClusterError("Cannot fallback from master");
        case ClusterHostType::kSyncSlave:
        case ClusterHostType::kSlave:
            return ClusterHostType::kMaster;
        case ClusterHostType::kSlaveOrMaster:
        case ClusterHostType::kNone:
        case ClusterHostType::kRoundRobin:
        case ClusterHostType::kNearest:
            throw ClusterError("Invalid ClusterHostType value for fallback " + ToString(host_type));
    }
    UINVARIANT(false, "Unexpected cluster host type");
}

bool IsRttAcceptable(Rtt rtt, Rtt max_rtt) noexcept { return rtt >= Rtt::zero() && rtt <= max_rtt; }

}  // namespace

topology::TopologyBase::DsnIndicesByType::const_iterator ResolveHostRole(
    const topology::TopologyBase::DsnIndicesByType& dsn_indices_by_type,
    ClusterHostType requested_role
) {
    auto host_role = requested_role;
    auto dsn_indices_it = dsn_indices_by_type.find(host_role);
    while (host_role != ClusterHostType::kMaster &&
           (dsn_indices_it == dsn_indices_by_type.end() || dsn_indices_it->second.indices.empty()))
    {
        const auto fallback_role = Fallback(host_role);
        LOG_WARNING() << "There is no pool for " << host_role << ", falling back to " << fallback_role;
        host_role = fallback_role;
        dsn_indices_it = dsn_indices_by_type.find(host_role);
    }

    if (dsn_indices_it == dsn_indices_by_type.end() || dsn_indices_it->second.indices.empty()) {
        throw ClusterUnavailable(
            fmt::format("Pool for {} (requested: {}) is not available", ToString(host_role), ToString(requested_role))
        );
    }
    return dsn_indices_it;
}

std::size_t SelectDsnIndex(
    const topology::TopologyBase::DsnIndices& dsn_indices,
    ClusterHostTypeFlags flags,
    std::atomic<std::uint32_t>& rr_host_idx
) {
    UASSERT(!dsn_indices.indices.empty());
    UASSERT(dsn_indices.nearest.has_value());

    if (dsn_indices.indices.empty()) {
        throw ClusterError("Cannot select host from an empty list");
    }

    const auto strategy_flags = flags & kClusterHostStrategyMask;
    LOG_TRACE() << "Applying " << strategy_flags << " strategy";

    if (!strategy_flags || strategy_flags == ClusterHostType::kRoundRobin) {
        const auto& indices = dsn_indices.GetRoundRobinIndices();
        std::size_t idx_pos = 0;
        if (indices.size() != 1) {
            idx_pos = rr_host_idx.fetch_add(1, std::memory_order_relaxed) % indices.size();
        }
        return indices[idx_pos];
    }

    if (strategy_flags == ClusterHostType::kNearest) {
        if (!dsn_indices.nearest.has_value()) {
            throw ClusterError("Nearest host is unknown");
        }
        return dsn_indices.nearest.value();
    }

    throw LogicError(fmt::format("Invalid strategy requested: {}, ensure only one is used", ToString(strategy_flags)));
}

void FillDsnIndices(
    topology::TopologyBase::DsnIndices& dsn_indices,
    std::span<const Rtt> roundtrip_times,
    std::optional<Rtt> max_rtt
) {
    const auto& indices = dsn_indices.indices;
    const auto get_rtt = [roundtrip_times](topology::TopologyBase::DsnIndex index) {
        UASSERT(index < roundtrip_times.size());
        return roundtrip_times[index];
    };

    dsn_indices.nearest.reset();
    dsn_indices.acceptable_indices.clear();
    dsn_indices.acceptable_indices.reserve(indices.size());

    for (const auto index : indices) {
        const auto rtt = get_rtt(index);
        if (!dsn_indices.nearest || rtt < get_rtt(*dsn_indices.nearest)) {
            dsn_indices.nearest = index;
        }
        if (max_rtt && IsRttAcceptable(rtt, *max_rtt)) {
            dsn_indices.acceptable_indices.push_back(index);
        }
    }
}

}  // namespace storages::postgres::detail

USERVER_NAMESPACE_END
