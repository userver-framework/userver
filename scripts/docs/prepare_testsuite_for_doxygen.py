#!/usr/bin/env python3
"""Prepare yandex-taxi-testsuite sources for Doxygen INPUT.

Injects a Doxygen group note into existing Python docstrings of
``@pytest.fixture`` functions and converts common Sphinx markup
(``:param:``, ``:returns:``, ``.. code-block::``, etc.) to Doxygen
commands so fixtures are extracted with EXTRACT_ALL=NO and
PYTHON_DOCSTRING=NO. Fixtures without a docstring are left untouched.
"""

from __future__ import annotations

import argparse
import ast
from pathlib import Path
import re
import sys
import textwrap

GITHUB_BLOB = 'https://github.com/yandex/yandex-taxi-testsuite/blob/develop/testsuite'

_CODE_BLOCK_RE = re.compile(r'^(\s*)\.\.\s+code-block::\s*(\S*)\s*$')
_LITERAL_BLOCK_RE = re.compile(r'^(.*?)\s*::\s*$')
_FIELD_PARAM_RE = re.compile(r'^(\s*):param\s+(?:(\S+)\s+)?(\w+)\s*:\s*(.*)$')
_FIELD_RETURNS_RE = re.compile(r'^(\s*):returns?\s*:\s*(.*)$')
_FIELD_RTYPE_RE = re.compile(r'^(\s*):rtype\s*:\s*(.*)$')
_FIELD_RAISES_RE = re.compile(r'^(\s*):raises?\s+(\w+)\s*:\s*(.*)$')
_FIELD_TYPE_RE = re.compile(r'^(\s*):type\s+(\w+)\s*:\s*(.*)$')
# e.g. mistyped ":spawn spawn: ..." → treat as @param
_FIELD_TYPO_PARAM_RE = re.compile(r'^(\s*):(\w+)\s+(\w+)\s*:\s*(.*)$')
_INLINE_ROLE_RE = re.compile(
    r':(?:py:)?(?:class|func|meth|attr|exc|data|mod|obj|ref):`([^`]+)`',
)
_DOUBLE_BACKTICK_RE = re.compile(r'``([^`]+)``')
_KNOWN_FIELDS = frozenset({
    'param',
    'type',
    'return',
    'returns',
    'rtype',
    'raise',
    'raises',
})
_CODE_LANG_MAP = {
    'python': 'py',
    'py': 'py',
    'yaml': 'yaml',
    'yml': 'yaml',
    'json': 'json',
    'bash': 'sh',
    'shell': 'sh',
    'sh': 'sh',
    'cpp': 'cpp',
    'c++': 'cpp',
}


def _is_pytest_fixture(decorator: ast.expr) -> bool:
    func = decorator.func if isinstance(decorator, ast.Call) else decorator
    if isinstance(func, ast.Attribute) and func.attr == 'fixture':
        return True
    return isinstance(func, ast.Name) and func.id == 'fixture'


def _docstring_expr(node: ast.AST) -> ast.Expr | None:
    if not isinstance(node, (ast.FunctionDef, ast.AsyncFunctionDef)):
        return None
    if not node.body:
        return None
    first = node.body[0]
    if not isinstance(first, ast.Expr):
        return None
    value = first.value
    if isinstance(value, ast.Constant) and isinstance(value.value, str):
        return first
    return None


def _quote_prefix(literal: str) -> str:
    for quote in ('"""', "'''", '"', "'"):
        if literal.startswith(quote):
            return quote
    raise ValueError(f'unsupported string literal: {literal[:20]!r}')


def _already_documented(literal: str) -> bool:
    return '@ingroup userver_testsuite_fixtures' in literal or 'Part of the [yandex-taxi-testsuite]' in literal


def _is_indented_more(line: str, indent: str) -> bool:
    if not line.strip():
        return True
    if indent:
        return line.startswith(indent + ' ') or line.startswith(indent + '\t')
    return line.startswith(' ') or line.startswith('\t')


def _collect_indented_block(lines: list[str], start: int, indent: str) -> tuple[list[str], int]:
    """Collect an RST indented block starting at *start*; return (raw_lines, next_index)."""
    i = start
    if i < len(lines) and lines[i].strip() == '':
        i += 1

    raw: list[str] = []
    while i < len(lines):
        line = lines[i]
        if line.strip() == '':
            j = i + 1
            while j < len(lines) and lines[j].strip() == '':
                j += 1
            if j < len(lines) and _is_indented_more(lines[j], indent) and lines[j].strip():
                raw.append(line)
                i += 1
                continue
            break
        if _is_indented_more(line, indent) and line.strip():
            raw.append(line)
            i += 1
            continue
        break
    return raw, i


def _dedent_block(raw_lines: list[str], indent: str) -> list[str]:
    if not raw_lines:
        return []
    stripped = []
    for line in raw_lines:
        if not line.strip():
            stripped.append('')
            continue
        if indent and line.startswith(indent):
            stripped.append(line[len(indent) :])
        else:
            stripped.append(line)
    return textwrap.dedent('\n'.join(stripped)).split('\n')


def _convert_inline(text: str) -> str:
    text = _INLINE_ROLE_RE.sub(r'@ref \1', text)
    text = _DOUBLE_BACKTICK_RE.sub(r'@c \1', text)
    return text


def _convert_field_line(line: str) -> str | None:
    """Return converted line, '' to drop the line, or None if not a field list line."""
    match = _FIELD_PARAM_RE.match(line)
    if match:
        indent, _type, name, desc = match.groups()
        return f'{indent}@param {name} {_convert_inline(desc)}'.rstrip()

    match = _FIELD_RETURNS_RE.match(line)
    if match:
        indent, desc = match.groups()
        return f'{indent}@returns {_convert_inline(desc)}'.rstrip()

    match = _FIELD_RTYPE_RE.match(line)
    if match:
        indent, desc = match.groups()
        return f'{indent}@returns {_convert_inline(desc)}'.rstrip()

    match = _FIELD_RAISES_RE.match(line)
    if match:
        indent, exc, desc = match.groups()
        return f'{indent}@throws {exc} {_convert_inline(desc)}'.rstrip()

    match = _FIELD_TYPE_RE.match(line)
    if match:
        return ''

    match = _FIELD_TYPO_PARAM_RE.match(line)
    if match:
        indent, field, name, desc = match.groups()
        if field in _KNOWN_FIELDS:
            return None
        return f'{indent}@param {name} {_convert_inline(desc)}'.rstrip()

    return None


def sphinx_docstring_to_doxygen(content: str) -> str:
    """Convert common Sphinx/RST docstring markup to Doxygen commands."""
    lines = content.split('\n')
    out: list[str] = []
    i = 0
    while i < len(lines):
        line = lines[i]

        code_match = _CODE_BLOCK_RE.match(line)
        if code_match:
            indent, lang = code_match.groups()
            raw, i = _collect_indented_block(lines, i + 1, indent)
            code = _dedent_block(raw, indent)
            mapped = _CODE_LANG_MAP.get(lang.lower(), lang) if lang else ''
            tag = f'@code{{.{mapped}}}' if mapped else '@code'
            out.append(f'{indent}{tag}')
            for row in code:
                out.append(f'{indent}{row}' if row else '')
            out.append(f'{indent}@endcode')
            continue

        # RST literal block: "text::" followed by an indented block.
        lit_match = _LITERAL_BLOCK_RE.match(line)
        if lit_match and not line.lstrip().startswith(('..', ':')) and i + 1 < len(lines):
            indent_match = re.match(r'^(\s*)', line)
            assert indent_match is not None
            indent = indent_match.group(1)
            peek = i + 1
            while peek < len(lines) and lines[peek].strip() == '':
                peek += 1
            if peek < len(lines) and _is_indented_more(lines[peek], indent) and lines[peek].strip():
                prefix = lit_match.group(1).rstrip()
                if prefix:
                    out.append(_convert_inline(prefix))
                raw, i = _collect_indented_block(lines, i + 1, indent)
                code = _dedent_block(raw, indent)
                out.append(f'{indent}@code')
                for row in code:
                    out.append(f'{indent}{row}' if row else '')
                out.append(f'{indent}@endcode')
                continue

        field = _convert_field_line(line)
        if field is not None:
            if field != '':
                out.append(field)
            i += 1
            continue

        out.append(_convert_inline(line))
        i += 1

    while out and out[-1] == '':
        out.pop()
    return '\n'.join(out)


def _inject_into_literal(literal: str, body_indent: str, url: str) -> str:
    if _already_documented(literal):
        return literal

    quote = _quote_prefix(literal)
    if not literal.endswith(quote):
        return literal

    out_quote = quote
    if quote in ('"', "'"):
        out_quote = '"""' if quote == '"' else "'''"

    # Source segment includes indent before the closing quotes; strip it.
    content = literal[len(quote) : -len(quote)]
    content = re.sub(r'[ \t]+$', '', content)
    content = sphinx_docstring_to_doxygen(content)

    note = (
        f'{body_indent}@ingroup userver_testsuite_fixtures\n{body_indent}Part of the [yandex-taxi-testsuite]({url})\n'
    )

    if not content.strip():
        new_content = f'\n{note}{body_indent}'
    else:
        trimmed = content.rstrip('\n')
        new_content = f'{trimmed}\n\n{note}{body_indent}'

    return f'{out_quote}{new_content}{out_quote}'


def _offset(text: str, lineno: int, col_offset: int) -> int:
    lines = text.splitlines(keepends=True)
    return sum(len(line) for line in lines[: lineno - 1]) + col_offset


def ensure_fixture_docs(path: Path, package_root: Path) -> int:
    text = path.read_text()
    try:
        tree = ast.parse(text)
    except SyntaxError as exc:
        print(f'warning: skip {path}: {exc}', file=sys.stderr)
        return 0

    rel = path.relative_to(package_root).as_posix()
    edits: list[tuple[int, int, str, str]] = []

    for node in ast.walk(tree):
        if not isinstance(node, (ast.FunctionDef, ast.AsyncFunctionDef)):
            continue
        if not any(_is_pytest_fixture(d) for d in node.decorator_list):
            continue
        doc_expr = _docstring_expr(node)
        if doc_expr is None:
            continue

        const = doc_expr.value
        assert isinstance(const, ast.Constant)
        if const.end_lineno is None or const.end_col_offset is None:
            continue

        literal = ast.get_source_segment(text, const)
        if literal is None or _already_documented(literal):
            continue

        body_indent = ' ' * const.col_offset
        url = f'{GITHUB_BLOB}/{rel}#L{node.lineno}'
        new_literal = _inject_into_literal(literal, body_indent, url)
        if new_literal == literal:
            continue

        start = _offset(text, const.lineno, const.col_offset)
        end = _offset(text, const.end_lineno, const.end_col_offset)
        edits.append((start, end, new_literal, node.name))

    if not edits:
        return 0

    edits.sort(key=lambda item: item[0], reverse=True)
    for start, end, new_literal, name in edits:
        text = text[:start] + new_literal + text[end:]
        print(f'documented: {rel}::{name}')

    path.write_text(text)
    return len(edits)


def main() -> int:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument(
        'testsuite_package_dir',
        type=Path,
        help='Path to the testsuite package directory (contains plugins/, databases/, ...)',
    )
    args = parser.parse_args()
    root: Path = args.testsuite_package_dir
    if not root.is_dir():
        print(f'error: {root} is not a directory', file=sys.stderr)
        return 1

    changed = 0
    for path in sorted(root.rglob('*.py')):
        if '__pycache__' in path.parts:
            continue
        changed += ensure_fixture_docs(path, root)

    print(f'prepare_testsuite_for_doxygen: updated {changed} fixture(s) under {root}')
    return 0


if __name__ == '__main__':
    sys.exit(main())
