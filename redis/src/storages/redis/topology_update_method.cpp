#include <userver/storages/redis/topology_update_method.hpp>

#include <userver/utils/trivial_map.hpp>

#include <string_view>

#include <userver/utils/assert.hpp>

USERVER_NAMESPACE_BEGIN

namespace storages::redis {

namespace {

constexpr utils::TrivialBiMap kToTopologyUpdateMethod = [](auto selector) {
    return selector()
        .Case("cluster_shards", TopologyUpdateMethod::kClusterShards)
        .Case("cluster_slots", TopologyUpdateMethod::kClusterSlots);
};

}  // namespace

TopologyUpdateMethod ToTopologyUpdateMethod(std::string_view view) {
    if (auto ret = kToTopologyUpdateMethod.TryFind(view); ret.has_value()) {
        return ret.value();
    }

    throw std::invalid_argument("Unknown topology update method: '" + std::string(view) + "'");
}

std::string_view ToStringView(TopologyUpdateMethod value) {
    auto ret = kToTopologyUpdateMethod.TryFind(value);
    UINVARIANT(ret.has_value(), fmt::format("Invalid topology update method {}", static_cast<uint64_t>(value)));
    return ret.value();
}

}  // namespace storages::redis

USERVER_NAMESPACE_END
