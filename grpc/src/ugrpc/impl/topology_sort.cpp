#include <ugrpc/impl/topology_sort.hpp>

#include <algorithm>
#include <unordered_set>

#include <fmt/ranges.h>
#include <boost/range/adaptor/map.hpp>

USERVER_NAMESPACE_BEGIN

namespace ugrpc::middlewares::impl {

std::vector<std::string> BuildTopologySortOfMiddlewares(
    std::unordered_map<std::string, std::vector<std::string>>&& graph
) {
    std::vector<std::string> topology_order{};
    topology_order.reserve(graph.size());
    std::unordered_set<std::string> without_connections{};
    while (!graph.empty()) {
        without_connections.clear();
        for (const auto& [node_name, connections] : graph) {
            if (connections.empty()) {
                without_connections.insert(node_name);
            }
        }
        if (without_connections.empty()) {
            throw std::runtime_error{fmt::format(
                "There are not nodes without connections => There is a cycle in the graph. Processed: {}. The cycle in "
                "the graph: {}",
                topology_order,
                graph | boost::adaptors::map_keys
            )};
        }
        for (const auto& node_name : without_connections) {
            graph.erase(node_name);
        }
        for (auto& [node, list] : graph) {
            list.erase(
                std::remove_if(
                    list.begin(),
                    list.end(),
                    [&without_connections](const auto& name) { return without_connections.count(name) != 0; }
                ),
                list.end()
            );
        }

        const auto prev_size = topology_order.size();
        topology_order.insert(
            topology_order.end(),
            std::make_move_iterator(without_connections.begin()),
            std::make_move_iterator(without_connections.end())
        );
        // sort only the last part of nodes
        std::sort(topology_order.begin() + prev_size, topology_order.end());
    }
    return topology_order;
}

}  // namespace ugrpc::middlewares::impl

USERVER_NAMESPACE_END
