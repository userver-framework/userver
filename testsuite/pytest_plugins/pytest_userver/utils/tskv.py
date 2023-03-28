import typing

TskvRow = typing.Dict[str, str]


def parse_line(line: str) -> TskvRow:
    parts = line.rstrip('\n').split('\t')
    if parts[:1] != ['tskv']:
        raise RuntimeError(f'Invalid tskv line: {line!r}')
    result = {}
    for part in parts[1:]:
        key, value = part.split('=', 1)
        result[key] = value
    return result
