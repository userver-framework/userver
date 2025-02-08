#pragma once

#include <string>
#include <unordered_map>
#include <vector>

USERVER_NAMESPACE_BEGIN

namespace ugrpc::middlewares::impl {

/// Sort the DAG from independent nodes (middlewares) to dependent.
/// If there is a group of independent middlewares, the func sorts this group by the lexicographic order,
/// so we guarantee a deterministic order for each call
std::vector<std::string> BuildTopologySortOfMiddlewares(
    std::unordered_map<std::string, std::vector<std::string>>&& graph
);

}  // namespace ugrpc::middlewares::impl

USERVER_NAMESPACE_END
