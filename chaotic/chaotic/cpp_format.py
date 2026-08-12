import re
import subprocess

SIMPLE_STYLE = '{BasedOnStyle: Google, ColumnLimit: 120}'

# Replace 3+ newlines with 2 newlines
_NEWLINE_RUN_RE = re.compile(r'[ ]*\n[ ]*\n[ \n]*\n([ ]*)', re.MULTILINE)

# 1) {% macro some_name %} and first JINJA block in it produce 8 unwanted spaces
# 2) whitespaces before new line are also removed
_LINE_TRIM_RE = re.compile(r'^ {1,8}|[ ]+$', re.MULTILINE)


def _collapse_whitespace(text: str) -> str:
    def _repl(match: re.Match[str]) -> str:
        trailing = match.group(1) or ''
        return '\n\n' + trailing

    return _NEWLINE_RUN_RE.sub(_repl, text)


def format_cpp_source_naive(text: str) -> str:
    if not text:
        return ''

    output = _collapse_whitespace(text)
    return _LINE_TRIM_RE.sub('', output)


def format_pp(input_: str, *, binary: str) -> str:
    if not binary:
        return format_cpp_source_naive(input_)
    output = subprocess.check_output([binary, '--style', SIMPLE_STYLE], input=input_, encoding='utf-8')
    return output + '\n'
