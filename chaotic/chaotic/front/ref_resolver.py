import os
import collections
from typing import Any, Dict, List, Optional, Set

from chaotic import error
from chaotic.front import types


class ResolverError(Exception):
    pass


def sort_dfs(nodes: Set[str], edges: Dict[str, List[str]]) -> List[str]:
    visited = set()
    visiting: List[str] = []
    sorted_nodes = []

    def do_node(node: str):
        if node in visiting:
            raise ResolverError('$ref cycle: ' + ', '.join(visiting))
        if node in visited:
            return

        visited.add(node)
        visiting.append(node)
        for subnode in edges[node]:
            do_node(subnode)

        visiting.pop()
        sorted_nodes.append(node)

    for node in sorted(nodes):
        do_node(node)

    return sorted_nodes


def normalize_ref(ref: str, base: str = None) -> str:
    """
    Normalizes a link, converting relative paths to canonical form.
    If the link contains an anchor ("#/"):
    - If the part before '#' is empty and base is specified, then base is used.
    - Otherwise, the part of the path is normalized via os.path.normpath.
    Examples:
    "../role.yaml#/definitions/role" -> "role.yaml#/definitions/role"
    "#/definitions/type" (with base="vfull") -> "vfull#/definitions/type"
    """
    if '#/' in ref:
        file_path, fragment = ref.split('#', 1)
        if not file_path and base:
            file_path = base
        else:
            file_path = os.path.normpath(file_path)
        return file_path + '#' + fragment
    return os.path.normpath(ref)


class RefResolver:
    def sort_schemas(
        self,
        schemas: types.ParsedSchemas,
        external_schemas: types.ResolvedSchemas = types.ResolvedSchemas(
            schemas={},
        ),
    ) -> types.ResolvedSchemas:
        """
        Sorts already parsed schemas. It is required for e.g. C++ translator.
        """
        edges = collections.defaultdict(list)
        nodes = set()
        name = ''

        def visitor(
            local_schema: types.Schema,
            parent: Optional[types.Schema],
        ) -> None:
            if not isinstance(local_schema, types.Ref):
                return

            cur_node: types.Schema = local_schema
            seen = set()
            indirect = False
            is_external = False
            while isinstance(cur_node, types.Ref):
                if cur_node.indirect:
                    indirect = True

                norm_ref = normalize_ref(cur_node.ref, base=name.split('#')[0] if cur_node.ref.startswith('#') else None)
                if norm_ref not in schemas.schemas:
                    ref = external_schemas.schemas.get(norm_ref)
                    if ref:
                        cur_node = ref
                        is_external = True
                    else:
                        known = '\n'.join([f'- {v}' for v in schemas.schemas.keys()])
                        known += '\n'.join([f'- {v}' for v in external_schemas.schemas.keys()])
                        raise Exception(
                            f'$ref to unknown type "{norm_ref}", known refs:\n{known}',
                        )
                else:
                    cur_node = schemas.schemas[norm_ref]
                if cur_node in seen:
                    # cycle is detected
                    # an exception will be raised later in sort_dfs()
                    break
                seen.add(cur_node)
            local_schema.schema = cur_node
            if indirect:
                local_schema.indirect = indirect

            if isinstance(parent, types.Array):
                if name == normalize_ref(local_schema.ref, base=name.split('#')[0] if local_schema.ref.startswith('#') else None):
                    if indirect:
                        raise error.BaseError(
                            full_filepath=local_schema.source_location().filepath,
                            infile_path=local_schema.source_location().location,
                            schema_type='jsonschema',
                            msg='Extra "x-usrv-cpp-indirect" for array\'s items, it is redundant.',
                        )

                    # self-referencing through array is explicitly allowed
                    # in C++ it is not aggregation, but std::vector<T>
                    local_schema.self_ref = True
                    return

            indirect = local_schema.x_properties.get(
                'x-usrv-cpp-indirect',
                local_schema.x_properties.get('x-taxi-cpp-indirect', False),
            )
            if not indirect:
                if not is_external:
                    if local_schema.ref.startswith('#'):
                        edges[name].append(normalize_ref(local_schema.ref, base=name.split('#')[0]))
                    else:
                        edges[name].append(normalize_ref(local_schema.ref))
            else:
                # skip indirect link
                pass

        for name, schema_item in schemas.schemas.items():
            visitor(schema_item, None)
            schema_item.visit_children(visitor)
            nodes.add(name)

        sorted_nodes = sort_dfs(nodes, edges)

        sorted_schemas = types.ResolvedSchemas(schemas={})
        for node in sorted_nodes:
            sorted_schemas.schemas[node] = schemas.schemas[node]
        return sorted_schemas

    @classmethod
    def _search_refs(cls, data: Any, *, inside_items: bool):
        if isinstance(data, list):
            for item in data:
                yield from cls._search_refs(item, inside_items=False)
        elif isinstance(data, dict):
            ref = data.get('$ref')
            if (
                ref is not None
                and 'x-usrv-cpp-indirect' not in data
                and 'x-taxi-cpp-indirect' not in data
                and not inside_items
            ):
                yield ref
            for key, value in data.items():
                yield from cls._search_refs(
                    value,
                    inside_items=(key == 'items'),
                )

    def sort_json_types(
        self,
        types: Dict[str, Any],
        erase_path_prefix: str = '',
    ) -> Dict[str, Any]:
        """
        Sorts not-yet-parsed schemas. Required for correct allOf/oneOf parsing.
        """
        nodes = []
        edges = collections.defaultdict(list)

        for name, value in types.items():
            nodes.append(name.rstrip('/'))

            refs = self._search_refs(value, inside_items=False)
            for ref in refs:
                if ref.startswith('#/'):
                    edges[name.rstrip('/')].append(erase_path_prefix + ref[1:])

        sorted_nodes = sort_dfs(set(nodes), edges)

        return {key + '/': types[key + '/'] for key in sorted_nodes}
