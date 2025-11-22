"""Collect vanilla types."""

from proto_structs.models import gen_node
from proto_structs.models import names


def collect_vanilla_types(*, types: list[gen_node.TypeNode]) -> list[gen_node.VanillaTypeDeclaration]:
    vanillas: list[gen_node.VanillaTypeDeclaration] = []
    for type_node in types:
        for child in type_node.iter_all_children():
            if isinstance(child, gen_node.StructNode):
                vanilla_name: names.TypeName = names.get_vanilla_fwd_type_name(name=child.vanilla_name)
                vanillas.append(gen_node.VanillaClassDeclaration(vanilla_name=vanilla_name))
    return vanillas
