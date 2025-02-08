#include <ugrpc/impl/middlewares_graph.hpp>

#include <fmt/format.h>
#include <fmt/ranges.h>

#include <ugrpc/impl/topology_sort.hpp>

#include <userver/ugrpc/server/middlewares/groups.hpp>
#include <userver/utils/text_light.hpp>

USERVER_NAMESPACE_BEGIN

namespace ugrpc::middlewares::impl {

namespace {

bool IsUserverGroup(const std::string& group) {
    return utils::text::EndsWith(group, "#end") || utils::text::EndsWith(group, "#begin");
}

void ValidateConnects(
    const MiddlewareDependency& dep,
    const std::vector<impl::Connect>& connects,
    const Dependencies& dependencies
) {
    for (const auto& con : connects) {
        if (IsUserverGroup(con.node_name)) {
            continue;
        }

        const auto it = dependencies.find(con.node_name);
        if (it != dependencies.end()) {
            if (dep.group.has_value() && it->second.group.has_value()) {
                UINVARIANT(
                    *dep.group == *it->second.group,
                    fmt::format(
                        "Middleware '{}' in group '{}'. Middleware '{}' in group '{}'. But they must be in the same "
                        "group.",
                        dep.middleware_name,
                        *dep.group,
                        con.node_name,
                        *it->second.group

                    )
                );
            }
        } else {
            UINVARIANT(
                con.type != DependencyType::kStrong,
                fmt::format(
                    "Middleware `{}` does not exist and there is a kStrong dependency from {}.",
                    con.node_name,
                    dep.middleware_name
                )
            );
        }
    }
}

void ValidateDependencies(const Dependencies& dependencies) {
    for (const auto& [name, dep] : dependencies) {
        UASSERT(name == dep.middleware_name);
        ValidateConnects(dep, dep.afters, dependencies);
        ValidateConnects(dep, dep.befores, dependencies);
    }
}

void AddEdges(Graph& graph, const MiddlewareDependency& dep) {
    for (const auto& before : dep.befores) {
        graph.AddEdge({before.node_name, dep.middleware_name, before.type});
    }
    for (const auto& after : dep.afters) {
        graph.AddEdge({dep.middleware_name, after.node_name, after.type});
    }
    graph.AddNode(dep.middleware_name, dep.enabled);
}

template <typename Group>
void AddEdgesForGroup(Graph& graph, const MiddlewareDependency& dep) {
    const auto begin = impl::BeginOfGroup<Group>();
    const auto end = impl::EndOfGroup<Group>();
    graph.AddNode(begin, true);
    graph.AddNode(end, true);
    graph.AddEdge({end, begin, DependencyType::kStrong});

    UASSERT(dep.afters.size() <= 1 && dep.befores.empty());
    if (dep.afters.size() == 1) {
        const auto after_end = dep.afters.front().node_name;
        graph.AddEdge({begin, after_end, DependencyType::kStrong});
    }
}

template <typename Group>
void AddEdgesGroup(Graph& graph) {
    const MiddlewareDependency dep = MiddlewareDependencyBuilder{Group::kDependency}.Extract(Group::kName);
    AddEdgesForGroup<Group>(graph, dep);
}

}  // namespace

void Graph::AddEdge(Edge edge) {
    const auto edge_id = edges_.size();
    edges_.push_back(edge);
    edges_lists_[edge.to].push_back(edge_id);
}

std::vector<Graph::Node> Graph::TopologySort() && { return BuildTopologySortOfMiddlewares(Reverse()); }

const Graph::Edge& Graph::GetEdge(EdgeId edge_id) const {
    UASSERT(edge_id < edges_.size());
    return edges_[edge_id];
}

std::unordered_map<std::string, std::vector<std::string>> Graph::Reverse() {
    std::unordered_map<std::string, std::vector<std::string>> res{};
    for (const auto& [name, _] : nodes_) {
        res.emplace(name, std::vector<Node>{});
    }
    for (const auto& [node, edges] : edges_lists_) {
        for (const auto& edge_id : edges) {
            const auto& edge = GetEdge(edge_id);
            const auto it = nodes_.find(edge.to);
            UINVARIANT(it != nodes_.end(), fmt::format("Middleware '{}' does not exist.", edge.to));
            if (!it->second) {  // The middleware is disabled
                UINVARIANT(
                    edge.type != DependencyType::kStrong,
                    fmt::format(
                        "There is a strong connect from '{}' to '{}'. But the last is missing.", edge.from, edge.to
                    )
                );
                // We can handle the weak connection => not panic
            }
            res[edge.from].push_back(edge.to);
        }
    }
    return res;
}

MiddlewareOrderedList BuildPipeline(Dependencies&& dependencies) {
    ValidateDependencies(dependencies);

    Graph graph{};

    AddEdgesGroup<server::groups::PreCore>(graph);
    AddEdgesGroup<server::groups::Logging>(graph);
    AddEdgesGroup<server::groups::Auth>(graph);
    AddEdgesGroup<server::groups::Core>(graph);
    AddEdgesGroup<server::groups::PostCore>(graph);
    AddEdgesGroup<server::groups::User>(graph);

    for (const auto& [name, dep] : dependencies) {
        AddEdges(graph, dep);
    }

    const auto nodes = std::move(graph).TopologySort();
    LOG_INFO() << fmt::format("The global middlewares configuration: [{}].", fmt::join(nodes, ", "));

    MiddlewareOrderedList list{};
    list.reserve(nodes.size());
    for (auto& mid : nodes) {
        if (IsUserverGroup(mid)) {
            continue;
        }
        const auto it = dependencies.find(mid);
        UINVARIANT(it != dependencies.end(), fmt::format("Middleware {} does not exist.", mid));
        list.push_back({mid, it->second.enabled});
    }

    return list;
}

}  // namespace ugrpc::middlewares::impl

USERVER_NAMESPACE_END
