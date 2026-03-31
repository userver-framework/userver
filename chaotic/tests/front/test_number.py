import pytest

from chaotic.front import parser as front_parser
from chaotic.front import types as front_types


def test_integer(simple_parse):
    parsed = simple_parse({'type': 'integer'})
    assert parsed.schemas == {'vfull#/definitions/type': front_types.Integer()}


def test_number(simple_parse):
    parsed = simple_parse({'type': 'number'})
    assert parsed.schemas == {'vfull#/definitions/type': front_types.Number()}


@pytest.mark.parametrize('format_', ['float', 'double'])
def test_format(simple_parse, format_):
    parsed = simple_parse({'type': 'number', 'format': format_})
    assert parsed.schemas == {
        'vfull#/definitions/type': front_types.Number(format=format_),
    }


def test_number_minmax(simple_parse):
    parsed = simple_parse({'type': 'number', 'minimum': 1, 'maximum': 2.2})
    assert parsed.schemas == {
        'vfull#/definitions/type': front_types.Number(minimum=1, maximum=2.2),
    }


def test_number_minmax_exclusive(simple_parse):
    parsed = simple_parse({
        'type': 'number',
        'exclusiveMinimum': 1,
        'exclusiveMaximum': 2.2,
    })
    assert parsed.schemas == {
        'vfull#/definitions/type': front_types.Number(
            exclusiveMinimum=1,
            exclusiveMaximum=2.2,
        ),
    }


def test_number_extra_enum(simple_parse):
    with pytest.raises(front_parser.ParserError) as exc:
        simple_parse({'type': 'number', 'enum': [1.0]})
    assert exc.value.infile_path == '/definitions/type'
    assert 'Unknown field: "enum"' in exc.value.msg


def test_integer_min_max(simple_parse):
    parsed = simple_parse({'type': 'integer', 'minimum': 1, 'maximum': 10})
    assert parsed.schemas == {
        'vfull#/definitions/type': front_types.Integer(minimum=1, maximum=10),
    }


def test_integer_minmax_exclusive(simple_parse):
    parsed = simple_parse({
        'type': 'integer',
        'exclusiveMinimum': 1,
        'exclusiveMaximum': 10,
    })
    assert parsed.schemas == {
        'vfull#/definitions/type': front_types.Integer(
            exclusiveMinimum=1,
            exclusiveMaximum=10,
        ),
    }


def test_integer_min_max_number(simple_parse):
    with pytest.raises(front_parser.ParserError) as exc:
        simple_parse({'type': 'integer', 'minimum': 1.1})
    assert exc.value.infile_path == '/definitions/type/minimum'
    assert exc.value.msg == 'Integer type is expected, 1.1 is found'


def test_integer_enum(simple_parse):
    parsed = simple_parse({'type': 'integer', 'enum': [1, 2, 3]})
    assert parsed.schemas == {
        'vfull#/definitions/type': front_types.Integer(enum=[1, 2, 3]),
    }


def test_integer_enum_wrong_type(simple_parse):
    with pytest.raises(front_parser.ParserError) as exc:
        simple_parse({'type': 'integer', 'enum': ['1']})
    assert exc.value.infile_path == '/definitions/type/enum/0'
    assert exc.value.msg == 'Integer type is expected, 1 is found'


def test_integer_min_wrong_str(simple_parse):
    with pytest.raises(front_parser.ParserError) as exc:
        simple_parse({'type': 'integer', 'minimum': '1'})
    assert exc.value.infile_path == '/definitions/type/minimum'
    assert exc.value.msg == 'Integer type is expected, 1 is found'
