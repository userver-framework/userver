import collections
from typing import Any
from typing import Dict
from typing import List
from typing import Optional
from typing import Set

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
            local_schema: types.Schema, parent: Optional[types.Schema],
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

                if cur_node.ref not in schemas.schemas:
                    ref = external_schemas.schemas.get(cur_node.ref)
                    if ref:
                        cur_node = ref
                        is_external = True
                    else:
                        known = '\n'.join([
                            f'- {v}' for v in schemas.schemas.keys()
                        ])
                        known += '\n'.join([
                            f'- {v}' for v in external_schemas.schemas.keys()
                        ])
                        raise Exception(
                            f'$ref to unknown type "{cur_node.ref}", '
                            f'known refs:\n{known}',
                        )
                else:
                    cur_node = schemas.schemas[cur_node.ref]
                if cur_node in seen:
                    # cycle is detected
                    # an exception will be raised later in sort_dfs()
                    break
                seen.add(cur_node)
            local_schema.schema = cur_node
            if indirect:
                local_schema.indirect = indirect

            if isinstance(parent, types.Array):
                if name == local_schema.ref:
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
                    # print(f'add {name} -> {local_schema.ref}')
                    edges[name].append(local_schema.ref)
            else:
                # skip indirect link
                pass

        # TODO: forbid non-schemas/ $refs
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
                    value, inside_items=(key == 'items'),
                )

    def sort_json_types(
        self, types: Dict[str, Any], erase_path_prefix: str = '',
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
