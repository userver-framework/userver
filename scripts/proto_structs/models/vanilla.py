"""Collect vanilla types."""

from typing import List

from proto_structs.models import gen_node
from proto_structs.models import names


def collect_vanilla_types(*, types: List[gen_node.TypeNode]) -> List[gen_node.VanillaType]:
    vanillas: List[gen_node.VanillaType] = []
    for type_node in types:
        for child in type_node.iter_all_children():
            if isinstance(child, gen_node.StructNode):
                vanilla_name: names.TypeName = names.get_vanilla_type_name(name=child.vanilla_name)
                vanillas.append(gen_node.VanillaClass(vanilla_name=vanilla_name))
    return vanillas
