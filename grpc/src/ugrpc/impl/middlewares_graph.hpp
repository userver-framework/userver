#pragma once

#include <string>
#include <unordered_map>
#include <vector>

#include <userver/ugrpc/middlewares/pipeline.hpp>

USERVER_NAMESPACE_BEGIN

namespace ugrpc::middlewares::impl {

class Graph final {
public:
    using MiddlewareName = std::string;
    using Node = MiddlewareName;

    struct Edge final {
        Node from;
        Node to;
        DependencyType type;
    };

    Graph() = default;

    void AddEdge(Edge edge);

    void AddNode(Node node, bool enabled) { nodes_.emplace(node, enabled); }

    std::vector<Node> TopologySort() &&;

private:
    std::unordered_map<std::string, std::vector<std::string>> Reverse();

    using EdgeId = std::size_t;

    const Edge& GetEdge(EdgeId edge_id) const;

    std::vector<Edge> edges_{};
    std::unordered_map<Node, std::vector<EdgeId>> edges_lists_{};
    std::unordered_map<Node, bool> nodes_{};
};

MiddlewareOrderedList BuildPipeline(Dependencies&& dependencies);

}  // namespace ugrpc::middlewares::impl

USERVER_NAMESPACE_END
