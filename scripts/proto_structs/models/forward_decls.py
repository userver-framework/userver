"""Adds forward declarations for types where needed."""

from collections.abc import Sequence

from proto_structs.models import gen_node
from proto_structs.models import type_ref


def add_forward_declarations(root_nodes: Sequence[gen_node.CodegenNode]) -> None:
    """
    Set `should_forward_declare` on `TypeNode`s where it is needed to refer to a type defined later.
    `root_nodes` should already be topologically sorted, recursively, so the order should be as in the generated file.
    """
    resolver = _ForwardDeclarationsResolver(root_nodes)
    resolver.run()


class _ForwardDeclarationsResolver:
    def __init__(self, root_nodes: Sequence[gen_node.CodegenNode]) -> None:
        self._root_nodes = root_nodes
        self._types_by_names: dict[str, gen_node.TypeNode] = {
            child.full_cpp_name(): child for node in root_nodes for child in gen_node.iter_type_nodes(node)
        }
        # As we walk through nodes, recursively, in the order of their definition, we will mark them as defined.
        self._is_type_defined_so_far: dict[gen_node.TypeNode, bool] = {
            child: False for node in root_nodes for child in gen_node.iter_type_nodes(node)
        }

    def _do_run(self, node: gen_node.CodegenNode) -> None:
        for child in node.children:
            self._do_run(child)
        # The intent is to visit "own" type dependencies of `node` without its children,
        # but recursive visit is also OK, because all children are marked as defined at this point.
        for dep in node.type_dependencies():
            if dep_node := self._types_by_names.get(dep.type_reference.full_cpp_name()):
                if not self._is_type_defined_so_far[dep_node]:
                    assert dep.kind == type_ref.TypeDependencyKind.WEAK, (
                        '`nodes` should be topologically sorted, recursively, '
                        'before passing them to `add_forward_declarations`.'
                    )
                    dep_node.should_forward_declare = True
        if isinstance(node, gen_node.TypeNode):
            self._is_type_defined_so_far[node] = True

    def run(self) -> None:
        for node in self._root_nodes:
            self._do_run(node)
