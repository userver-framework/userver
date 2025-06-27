import collections
import re
from typing import Any
from typing import Dict
from typing import List
from typing import Tuple

from chaotic.front import parser as chaotic_parser
from chaotic.front import ref_resolver

REF_SHRINK_RE = re.compile('/[^/]+/../')


def normalize_ref(filepath: str, ref: str) -> str:
    if ref.startswith('#'):
        return filepath + ref

    return chaotic_parser.SchemaParser._normalize_ref(
        '{}/{}'.format(
            filepath.rsplit('/', 1)[0],
            ref,
        )
    )


# Extracts list of external (non-'#ref') $refs in global form
# (e.g. 'other.yaml#ref' becomes '/path/to/other.yaml#ref')
def _extract_refs(filepath: str, content: Any) -> List[str]:
    refs = []

    def visit(value: Any) -> None:
        if isinstance(value, list):
            for item in value:
                visit(item)
        if isinstance(value, dict):
            for item in value.values():
                visit(item)
            if '$ref' in value:
                ref = value['$ref']
                if not ref.startswith('#'):
                    refs.append(normalize_ref(filepath, ref).split('#')[0])

    visit(content)
    return refs


def sort_openapis(contents: Dict[str, Any]) -> List[Tuple[str, Any]]:
    nodes = set()

    edges = collections.defaultdict(list)
    for filepath, content in contents.items():
        nodes.add(filepath)

        refs = _extract_refs(filepath, content)
        for ref in refs:
            edges[filepath].append(ref)

    sorted_nodes = ref_resolver.sort_dfs(nodes, edges)
    return [(filepath, contents[filepath]) for filepath in sorted_nodes]
