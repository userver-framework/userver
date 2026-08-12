"""Print semicolon-separated operation relpaths from OpenAPI YAML files.

Output format matches CMake list conventions so the result can be used
directly with execute_process / OUTPUT_VARIABLE.
"""

import sys
from typing import Any

import yaml

_METHODS = frozenset(['get', 'post', 'put', 'delete', 'patch', 'head', 'options'])


def _make_op_relpath(path: str, method: str, op: dict[str, Any]) -> str:
    operation_id = op.get('operationId')
    if operation_id:
        return operation_id.lower()
    slug = path.strip('/').replace('/', '_')
    return f'{slug}/{method.lower()}'


def _collect_relpaths(yaml_path: str) -> list[str]:
    with open(yaml_path) as f:
        spec = yaml.safe_load(f)
    relpaths = []
    for path, methods in (spec.get('paths') or {}).items():
        for method, op in (methods or {}).items():
            if method not in _METHODS:
                continue
            if not isinstance(op, dict):
                continue
            relpaths.append(_make_op_relpath(path, method, op))
    return relpaths


def main() -> None:
    all_relpaths: list[str] = []
    for yaml_path in sys.argv[1:]:
        all_relpaths.extend(_collect_relpaths(yaml_path))
    print(';'.join(all_relpaths))


if __name__ == '__main__':
    main()
