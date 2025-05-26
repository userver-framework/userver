import re
from typing import Dict
from typing import Tuple

TskvRow = Dict[str, str]


_EQUALS_OR_BACKSLASH = re.compile('=|\\\\')
_UNESCAPED_CHARS = {
    'n': '\n',
    'r': '\r',
    't': '\t',
    '0': '\0',
    '\\': '\\',
    '=': '=',
}


def _search_equals_or_backslash(s: str, start: int) -> int:
    match = _EQUALS_OR_BACKSLASH.search(s, start)

    return match.start() if match else -1


def _search_backslash(s: str, start: int) -> int:
    return s.find('\\', start)


def _parse_pair(pair: str) -> Tuple[str, str]:
    search_fn = _search_equals_or_backslash
    key = None
    unescaped_part = ''

    start = 0
    end = search_fn(pair, start)
    while end != -1:
        unescaped_part += pair[start:end]
        if pair[end] == '=':
            key = unescaped_part
            unescaped_part = ''
            search_fn = _search_backslash
        elif end + 1 != len(pair) and pair[end + 1] in _UNESCAPED_CHARS:
            unescaped_part += _UNESCAPED_CHARS[pair[end + 1]]
            end += 1
        else:
            unescaped_part += '\\'

        start = end + 1
        end = search_fn(pair, start)

    unescaped_part += pair[start:]

    if key is None:
        raise RuntimeError(f'Invalid tskv pair: {pair}')

    return key, unescaped_part


def parse_line(line: str) -> TskvRow:
    parts = line.rstrip('\n').split('\t')
    if parts[:1] != ['tskv']:
        raise RuntimeError(f'Invalid tskv line: {line!r}')
    return dict(map(_parse_pair, parts[1:]))
