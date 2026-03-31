import pytest

from chaotic.front import parser as front_parser
from chaotic.front import types as front_types


def test_empty(simple_parse):
    simple_parse({
        'type': 'object',
        'properties': {},
        'additionalProperties': False,
    })


def test_very_empty(simple_parse):
    simple_parse({'type': 'object', 'additionalProperties': False})


def test_unknown_required(simple_parse):
    with pytest.raises(front_parser.ParserError) as exc:
        simple_parse({
            'type': 'object',
            'properties': {},
            'additionalProperties': False,
            'required': ['unknown'],
        })
    assert exc.value.infile_path == '/definitions/type/required'
    assert exc.value.msg == ('Field "unknown" is set in "required", but missing in "properties"')


def test_unknown_fields(simple_parse):
    with pytest.raises(front_parser.ParserError) as exc:
        simple_parse({
            'type': 'object',
            'unknown_field': 'x',
            'properties': {},
            'additionalProperties': False,
        })
    assert exc.value.infile_path == '/definitions/type'
    assert 'Unknown field: "unknown_field"' in exc.value.msg


def test_error_in_property(simple_parse):
    with pytest.raises(front_parser.ParserError) as exc:
        simple_parse({
            'type': 'object',
            'properties': {'field': {'type': 'xxxx'}},
            'additionalProperties': False,
        })
    assert exc.value.infile_path == '/definitions/type/properties/field/type'
    assert exc.value.msg == 'Unknown type "xxxx"'


def test_error_in_extra(simple_parse):
    with pytest.raises(front_parser.ParserError) as exc:
        simple_parse({
            'type': 'object',
            'properties': {},
            'additionalProperties': {'type': 'xxx'},
        })
    assert exc.value.infile_path == '/definitions/type/additionalProperties/type'
    assert exc.value.msg == 'Unknown type "xxx"'


def test_property_and_additional(simple_parse):
    data = simple_parse({
        'type': 'object',
        'properties': {'field': {'type': 'integer'}},
        'additionalProperties': {'type': 'boolean'},
    })
    assert data.schemas == {
        'vfull#/definitions/type': front_types.SchemaObject(
            properties={'field': front_types.Integer()},
            additionalProperties=front_types.Boolean(),
        ),
    }
