import pytest

from chaotic.front.parser import ParserError
from chaotic.front.types import Integer
from chaotic.front.types import Number


def test_integer(simple_parse):
    parsed = simple_parse({'type': 'integer'})
    assert parsed.schemas == {'vfull#/definitions/type': Integer()}


def test_number(simple_parse):
    parsed = simple_parse({'type': 'number'})
    assert parsed.schemas == {'vfull#/definitions/type': Number()}


@pytest.mark.parametrize('format_', ['float', 'double'])
def test_format(simple_parse, format_):
    parsed = simple_parse({'type': 'number', 'format': format_})
    assert parsed.schemas == {
        'vfull#/definitions/type': Number(format=format_),
    }


def test_number_minmax(simple_parse):
    parsed = simple_parse({'type': 'number', 'minimum': 1, 'maximum': 2.2})
    assert parsed.schemas == {
        'vfull#/definitions/type': Number(minimum=1, maximum=2.2),
    }


def test_number_minmax_exclusive(simple_parse):
    parsed = simple_parse({
        'type': 'number',
        'exclusiveMinimum': 1,
        'exclusiveMaximum': 2.2,
    })
    assert parsed.schemas == {
        'vfull#/definitions/type': Number(
            exclusiveMinimum=1, exclusiveMaximum=2.2,
        ),
    }


def test_number_extra_enum(simple_parse):
    try:
        simple_parse({'type': 'number', 'enum': [1.0]})
        assert False
    except ParserError as exc:
        assert exc.infile_path == '/definitions/type/enum'
        assert 'Unknown field: "enum"' in exc.msg


def test_integer_min_max(simple_parse):
    parsed = simple_parse({'type': 'integer', 'minimum': 1, 'maximum': 10})
    assert parsed.schemas == {
        'vfull#/definitions/type': Integer(minimum=1, maximum=10),
    }


def test_integer_minmax_exclusive(simple_parse):
    parsed = simple_parse({
        'type': 'integer',
        'exclusiveMinimum': 1,
        'exclusiveMaximum': 10,
    })
    assert parsed.schemas == {
        'vfull#/definitions/type': Integer(
            exclusiveMinimum=1, exclusiveMaximum=10,
        ),
    }


def test_integer_min_max_number(simple_parse):
    try:
        simple_parse({'type': 'integer', 'minimum': 1.1})
        assert False
    except ParserError as exc:
        assert exc.infile_path == '/definitions/type/minimum'
        assert exc.msg == 'field "minimum" has wrong type'


def test_integer_enum(simple_parse):
    parsed = simple_parse({'type': 'integer', 'enum': [1, 2, 3]})
    assert parsed.schemas == {
        'vfull#/definitions/type': Integer(enum=[1, 2, 3]),
    }


def test_integer_enum_wrong_type(simple_parse):
    try:
        simple_parse({'type': 'integer', 'enum': ['1']})
        assert False
    except ParserError as exc:
        assert exc.infile_path == '/definitions/type/enum'
        assert exc.msg == 'field "enum" contains non-integers (1)'


def test_integer_min_wrong_str(simple_parse):
    try:
        simple_parse({'type': 'integer', 'minimum': '1'})
        assert False
    except ParserError as exc:
        assert exc.infile_path == '/definitions/type/minimum'
        assert exc.msg == 'field "minimum" has wrong type'
