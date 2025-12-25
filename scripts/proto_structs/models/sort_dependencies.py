"""Graph of dependencies between types and a topology sort for that graph."""

from __future__ import annotations

from collections.abc import Iterable
from collections.abc import Sequence
import dataclasses
import graphlib
from typing import TypeAlias

from proto_structs.models import gen_node
from proto_structs.models import toposort
from proto_structs.models import type_ref
from proto_structs.models import type_ref_consts

#: Graph nodes are direct children of the scope (namespace or struct) that need to be sorted.
#:
#: "Node A depends on node B" means: a type defined in A (possibly A itself) refers to
#: a type defined in B (possibly B itself).
#:
#: Guideline: write the type of some `TypeNode` as `_GraphNode` if and only if it is directly a graph node
#: (not a nested type, not an unrelated type).
_GraphNode: TypeAlias = gen_node.TypeNode

_MAX_CYCLE_BREAK_ATTEMPTS = 1000


@dataclasses.dataclass
class _Cycle:
    #: A list of nodes forming a cycle, such that each node depends on the next node in the list.
    #: The last node is guaranteed to be a duplicate of the first node, to make it clear that the list is cyclic.
    nodes: list[_GraphNode]

    @staticmethod
    def from_exception(exception: graphlib.CycleError) -> _Cycle:
        """Extracts cycle info from exception."""
        cycle: list[_GraphNode] = exception.args[1]
        assert len(cycle) >= 2
        assert cycle[-1] is cycle[0]
        return _Cycle(nodes=list(reversed(cycle)))

    @property
    def edges(self) -> Iterable[tuple[_GraphNode, _GraphNode]]:
        """Graph edges (node -> dependency) that form the cycle."""
        return zip(self.nodes[:-1], self.nodes[1:], strict=True)


def _iter_struct_fields(node: gen_node.CodegenNode) -> Iterable[gen_node.StructField]:
    for child in node.iter_all_children():
        if isinstance(child, (gen_node.StructNode, gen_node.OneofNode)):
            yield from child.fields


class _Graph:
    def __init__(self, nodes: Sequence[_GraphNode]) -> None:
        # The nodes to sort. These are direct children of the scope (namespace or struct) that needs to be sorted.
        self._nodes = nodes
        # Allows to go from `TypeReference` to `TypeNode` for references to generated types.
        self._types_by_names: dict[str, gen_node.TypeNode] = {
            child.full_cpp_name(): child for node in nodes for child in gen_node.iter_type_nodes(node)
        }
        # A map from all nested type definitions (including self) to their (possibly indirect) parents from `nodes`.
        self._types_to_containing_graph_nodes: dict[gen_node.TypeNode, _GraphNode] = {
            child: node for node in nodes for child in gen_node.iter_type_nodes(node)
        }
        # Maps nodes to their dependencies, as required by `TopologicalSorter`.
        self._graph: dict[_GraphNode, set[_GraphNode]] = {}

    def prepare(self) -> None:
        for node in self._nodes:
            self._graph.setdefault(node, set())

        for node in self._nodes:
            for dep in self.iter_referenced_type_nodes(entity=node, entity_graph_node=node):
                self._graph[node].add(self._types_to_containing_graph_nodes[dep])

    def iter_referenced_type_nodes(
        self,
        *,
        entity: type_ref.HasTypeDependencies,
        entity_graph_node: _GraphNode,
    ) -> Iterable[gen_node.TypeNode]:
        for dep in entity.type_dependencies():
            # Select `TypeReference`s to types defined among `self._nodes`, get the corresponding `TypeNode`s.
            if dep_node := self._types_by_names.get(dep.type_reference.full_cpp_name()):
                dep_node_parent = self._types_to_containing_graph_nodes[dep_node]

                # Dependency within a graph node will be resolved while sorting the node itself; skip it for now.
                # However, cyclic dependency on the graph node itself needs to be accounted for.
                #
                # See examples in: libraries/proto-structs/codegen-tests/proto/box/autobox/dependency_on_nested.proto
                if dep_node_parent is entity_graph_node:
                    if dep_node is entity_graph_node and dep.kind == type_ref.TypeDependencyKind.STRONG:
                        yield dep_node
                    continue

                # Dependency on a nested type holds even when WEAK.
                # This is because the outer type needs to be defined to mention its nested type.
                #
                # See examples in: libraries/proto-structs/codegen-tests/proto/box/autobox/dependency_on_self.proto
                if dep_node != dep_node_parent:
                    yield dep_node
                    continue

                if dep.kind == type_ref.TypeDependencyKind.STRONG:
                    yield dep_node

    def can_break_dependency(self, node: _GraphNode, *, dependency: _GraphNode) -> bool:
        # Can break dependency if node only depends on the type itself, not on its nested types.
        # This is because nested types cannot be forward declared, e.g. mentioning Foo::Bar requires definition of Foo.
        #
        # See examples in: libraries/proto-structs/codegen-tests/proto/box/autobox/dependency_on_nested.proto
        return all(
            dep_node is dependency
            for dep_node in self.iter_referenced_type_nodes(entity=node, entity_graph_node=node)
            if self._types_to_containing_graph_nodes[dep_node] is dependency
        )

    def break_dependency(self, node: _GraphNode, *, dependency: _GraphNode) -> None:
        for field in _iter_struct_fields(node):
            for dep_node in self.iter_referenced_type_nodes(entity=field.field_type, entity_graph_node=node):
                if self._types_to_containing_graph_nodes[dep_node] is dependency:
                    if dep_node is dependency:
                        gen_node.wrap_field_in_box(field)
                    else:
                        field.field_type = type_ref_consts.UNBREAKABLE_DEPENDENCY_CYCLE

    def sort_nodes(self) -> list[_GraphNode]:
        attempts = 0
        while True:
            try:
                # Need to at least ensure codegen output stability.
                # Keeping the order close to the original proto file is also helpful.
                return toposort.stable_toposort(items=self._nodes, graph=self._graph)
            except graphlib.CycleError as exc:
                cycle = _Cycle.from_exception(exc)

            if attempts == _MAX_CYCLE_BREAK_ATTEMPTS:
                names = (node.full_cpp_name() for node in cycle.nodes)
                raise RuntimeError(f'Failed to break dependency cycle: {" -> ".join(names)}')
            attempts += 1

            can_break_dependencies = [
                self.can_break_dependency(node, dependency=dependency) for node, dependency in cycle.edges
            ]
            can_break_any_dependency = any(can_break_dependencies)
            for (node, dependency), can_break in zip(cycle.edges, can_break_dependencies, strict=True):
                if can_break or not can_break_any_dependency:
                    self.break_dependency(node, dependency=dependency)
                    self._graph[node].discard(dependency)


def sort_types_topologically(nodes: Sequence[gen_node.TypeNode]) -> list[gen_node.TypeNode]:
    """
    Topologically sort `nodes`.
    Wrap fields in `utils::Box` (as in `gen_node.wrap_field_in_box`) when necessary.
    As a last resort, replace the field type with `proto_structs::UnbreakableDependencyCycle`.
    """
    graph = _Graph(nodes)
    graph.prepare()
    return graph.sort_nodes()
