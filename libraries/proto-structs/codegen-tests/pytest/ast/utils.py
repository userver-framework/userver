import pathlib

import yatest.common


def load_proto_source_file(path: str) -> str:
    proper_path = pathlib.Path('..', '..', 'proto') / path
    resolved_path = pathlib.Path(yatest.common.test_source_path(str(proper_path)))
    return resolved_path.read_text(encoding='utf-8')
