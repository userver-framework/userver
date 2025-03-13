import os
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


def normalize_ref(ref: str) -> str:
    """
    Normalizes a link by converting relative paths to canonical form.
    For example, "../test.yaml#/definitions/test" becomes "test.yaml#/definitions/test".
    """
    if '#/' in ref:
        file_path, fragment = ref.split('#', 1)
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

        # Let's build a map: normalized value of $ref -> original value of the schema key
        norm_to_orig = {}
        for key in schemas.schemas.keys():
            norm_to_orig[normalize_ref(key)] = key

        def visitor(
            local_schema: types.Schema,
            parent: Optional[types.Schema],
        ) -> None:
            nonlocal name
            if not isinstance(local_schema, types.Ref):
                return

            cur_node: types.Schema = local_schema
            seen = set()
            indirect = False
            is_external = False
            while isinstance(cur_node, types.Ref):
                if cur_node.indirect:
                    indirect = True

                norm_ref = normalize_ref(cur_node.ref)
                if norm_ref not in norm_to_orig:
                    ref = external_schemas.schemas.get(norm_ref)
                    if ref:
                        cur_node = ref
                        is_external = True
                    else:
                        known = '\n'.join([f'- {v}' for v in norm_to_orig.keys()])
                        known += '\n' + '\n'.join([f'- {v}' for v in external_schemas.schemas.keys()])
                        raise Exception(
                            f'$ref to unknown type "{norm_ref}", known refs:\n{known}',
                        )
                else:
                    orig = norm_to_orig[norm_ref]
                    cur_node = schemas.schemas[orig]
                if cur_node in seen:
                    # cycle is detected; an exception will be raised later in sort_dfs()
                    break
                seen.add(cur_node)
            local_schema.schema = cur_node
            if indirect:
                local_schema.indirect = indirect

            if isinstance(parent, types.Array):
                if name == normalize_ref(local_schema.ref):
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
                    edges[name].append(normalize_ref(local_schema.ref))
            else:
                # skip indirect link
                pass

        for key, schema_item in schemas.schemas.items():
            name = normalize_ref(key)
            visitor(schema_item, None)
            schema_item.visit_children(visitor)
            nodes.add(name)

        sorted_nodes = sort_dfs(nodes, edges)

        sorted_schemas = types.ResolvedSchemas(schemas={})
        for node in sorted_nodes:
            orig = norm_to_orig[node]
            sorted_schemas.schemas[node] = schemas.schemas[orig]
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
            norm_name = normalize_ref(name.rstrip('/'))
            nodes.append(norm_name)

            refs = self._search_refs(value, inside_items=False)
            for ref in refs:
                if ref.startswith('#/'):
                    edges[norm_name].append(erase_path_prefix + ref[1:])

        sorted_nodes = sort_dfs(set(nodes), edges)

        return {key + '/': types[normalize_ref(key) + '/'] for key in sorted_nodes}
