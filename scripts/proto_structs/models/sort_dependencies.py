"""Graph of dependencies between types and a topology sort that graph."""

import graphlib
import typing
from typing import Dict
from typing import List
from typing import Set

from proto_structs.models import gen_node
from proto_structs.models import type_ref


def sort_types(nodes: List[gen_node.TypeNode]) -> List[gen_node.TypeNode]:
    types_by_names: Dict[str, gen_node.TypeNode] = {node.full_cpp_name(): node for node in nodes}

    graph: Dict[str, Set[str]] = {}

    for node in nodes:
        graph.setdefault(node.full_cpp_name(), set())

    for node in nodes:
        for dep in node.type_dependencies():
            if dep.kind == type_ref.TypeDependencyKind.STRONG:
                child = dep.type_reference
                graph[node.full_cpp_name()].add(child.full_cpp_name())

    try:
        ts = graphlib.TopologicalSorter(graph)
        top_sort = ts.static_order()
        return [types_by_names[node_name] for node_name in top_sort if node_name in types_by_names]
    except graphlib.CycleError as exc:
        cycle: List[str] = typing.cast(List[str], exc.args[1])
        raise Exception(f'A cycle {cycle} in graph: \n {graph}')
