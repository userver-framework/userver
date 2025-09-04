import collections
from typing import Any

from chaotic.front import parser as chaotic_parser
from chaotic.front import ref
from chaotic.front import ref_resolver


def normalize_ref(filepath: str, ref_: str) -> str:
    if not ref.Ref(ref_).file:
        return filepath + ref_

    return chaotic_parser.SchemaParser._normalize_ref(
        '{}/{}'.format(
            filepath.rsplit('/', 1)[0],
            ref_,
        )
    )


# Extracts list of external (non-'#ref') $refs in global form
# (e.g. 'other.yaml#ref' becomes '/path/to/other.yaml#ref')
def _extract_refs(filepath: str, content: Any) -> list[str]:
    refs = []

    def visit(value: Any) -> None:
        if isinstance(value, list):
            for item in value:
                visit(item)
        if isinstance(value, dict):
            for item in value.values():
                visit(item)
            if '$ref' in value:
                ref_ = value['$ref']
                reference = ref.Ref(ref_)
                if reference.file:
                    refs.append(ref.Ref(normalize_ref(filepath, ref_)).file)

    visit(content)
    return refs


def sort_openapis(contents: dict[str, Any]) -> list[tuple[str, Any]]:
    nodes = set()

    edges = collections.defaultdict(list)
    for filepath, content in contents.items():
        nodes.add(filepath)

        refs = _extract_refs(filepath, content)
        for ref_ in refs:
            edges[filepath].append(ref_)

    sorted_nodes = ref_resolver.sort_dfs(nodes, edges)
    return [(filepath, contents[filepath]) for filepath in sorted_nodes]
