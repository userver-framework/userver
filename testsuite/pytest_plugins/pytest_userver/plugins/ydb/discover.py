import pathlib


def find_schemas(schema_dirs: list[pathlib.Path]) -> list[pathlib.Path]:
    result = []
    for path in schema_dirs:
        if not path.is_dir():
            continue
        result.extend(_scan_path(path))
    return result


def _scan_path(schema_path: pathlib.Path) -> list[pathlib.Path]:
    result = []
    for entry in schema_path.iterdir():
        if entry.suffix == '.yaml' and entry.is_file():
            result.append(entry)
        elif entry.is_dir():
            result.extend(_scan_path(entry))
    return result
