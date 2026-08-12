import pytest
from pytest_userver.utils import tskv


@pytest.mark.parametrize(
    'tskv_line, result',
    [
        pytest.param(
            'tskv\thello=word\n',
            {'hello': 'word'},
            id='1_field',
        ),
        pytest.param(
            'tskv\thello=word\tfoo=bar\n',
            {'hello': 'word', 'foo': 'bar'},
            id='2_fields',
        ),
        pytest.param(
            'tskv\thello\\=there=word\tfoo\\=there=bar\n',
            {'hello=there': 'word', 'foo=there': 'bar'},
            id='2_field_and_eq_sign',
        ),
        pytest.param(
            'tskv\t\\=hello=word\t\\=foo\\=\\=there\\==bar\n',
            {'=hello': 'word', '=foo==there=': 'bar'},
            id='2_fields_and_many_eq_signs',
        ),
        pytest.param(
            'tskv\thello=\\t\\tw\\t\\to\\tr\\td\\t\\t\tfoo\\=there=\\tbar\n',
            {'hello': '\t\tw\t\to\tr\td\t\t', 'foo=there': '\tbar'},
            id='2_fields_and_many_tabs',
        ),
        pytest.param(
            'tskv\thello=\\n\\nw\\n\\no\\nr\\nd\\n\\n\tfoo\\=there=\\nbar\n',
            {'hello': '\n\nw\n\no\nr\nd\n\n', 'foo=there': '\nbar'},
            id='2_fields_and_many_newlines',
        ),
        pytest.param(
            'tskv\t=word\tfoo=\n',
            {'': 'word', 'foo': ''},
            id='3_fields_with_empty_key_values_fixed',
        ),
        pytest.param(
            'tskv\t\\===\tfoo==\t==\n',
            {'=': '=', 'foo': '=', '': '='},
            id='3_fields_with_empty_key_values_and_eq_signs',
        ),
        pytest.param(
            'tskv\n',
            {},
            id='empty_record',
        ),
    ],
)
def test_single_line(tskv_line, result):
    assert tskv.parse_line(tskv_line) == result


@pytest.mark.parametrize(
    'tskv_line, result',
    [
        pytest.param(
            'tskv\t"k"="v"\t"="\n',
            {'"k"': '"v"', '"': '"'},
            id='quotes',
        ),
        pytest.param(
            'tskv\t\\"k\\"=\\"v\\"\t\\"=\\"\n',
            {'\\"k\\"': '\\"v\\"', '\\"': '\\"'},
            id='quotes_backslashes',
        ),
        pytest.param(
            'tskv\t\\k=\\v\\\t\\\\=\\\n',
            {'\\k': '\\v\\', '\\': '\\'},
            id='backslashes',
        ),
        pytest.param(
            'tskv\t\\k=\\v\\\\\t\\\\=\\\\\n',
            {'\\k': '\\v\\', '\\': '\\'},
            id='backslashes_fixed',
        ),
        pytest.param(
            'tskv\t\\\\k=\\\\v\\\\\t=\\\\\n',
            {'\\k': '\\v\\', '': '\\'},
            id='backslashes_and_empty_fixed',
        ),
        pytest.param(
            'tskv\t\rk\r=\rv\r\t\r=\r\n',
            {'\rk\r': '\rv\r', '\r': '\r'},
            id='carriage_returns',
        ),
        pytest.param(
            'tskv\t\1k\1=\1v\1\t\1=\1\n',
            {'\u0001k\u0001': '\u0001v\u0001', '\u0001': '\u0001'},
            id='code_1_char',
        ),
        pytest.param(
            'tskv\t\\rk\\r=\\nv\\n\t\\n=\\b\n',
            {'\rk\r': '\nv\n', '\n': '\\b'},
            id='newline_escape_fixed',
        ),
        pytest.param(
            'tskv\t\x1fk\x1f=\x1fv\x1f\t\x1f=\x1f\n',
            {'\u001fk\u001f': '\u001fv\u001f', '\u001f': '\u001f'},
            id='control_char_1f',
        ),
    ],
)
def test_escape_single_line(tskv_line, result):
    assert tskv.parse_line(tskv_line) == result


@pytest.mark.parametrize(
    'tskv_line, result',
    [
        pytest.param(
            'tskv\t=world\n',
            {'': 'world'},
            id='key_missing',
        ),
        pytest.param(
            'tskv\thello=\n',
            {'hello': ''},
            id='value_missing',
        ),
        pytest.param(
            'tskv\t=\n',
            {'': ''},
            id='key_value_missing_equals',
        ),
        pytest.param(
            'tskv\tt=\tmodule=\tlevel=\n',
            {'t': '', 'module': '', 'level': ''},
            id='multiple_values_missing',
        ),
    ],
)
def test_single_line_empties(tskv_line, result):
    assert tskv.parse_line(tskv_line) == result
