import pytest

from chaotic.front.parser import ParserError
from chaotic.front.types import Boolean
from chaotic.front.types import Integer
from chaotic.front.types import SchemaObject


def test_empty(simple_parse):
    simple_parse({
        'type': 'object',
        'properties': {},
        'additionalProperties': False,
    })


def test_very_empty(simple_parse):
    simple_parse({'type': 'object', 'additionalProperties': False})


def test_unknown_required(simple_parse):
    with pytest.raises(ParserError) as exc:
        simple_parse({
            'type': 'object',
            'properties': {},
            'additionalProperties': False,
            'required': ['unknown'],
        })
    assert exc.value.infile_path == '/definitions/type/required'
    assert exc.value.msg == ('Field "unknown" is set in "required", but missing in "properties"')


def test_unknown_fields(simple_parse):
    with pytest.raises(ParserError) as exc:
        simple_parse({
            'type': 'object',
            'unknown_field': 'x',
            'properties': {},
            'additionalProperties': False,
        })
    assert exc.value.infile_path == '/definitions/type/unknown_field'
    assert 'Unknown field: "unknown_field"' in exc.value.msg


def test_error_in_property(simple_parse):
    with pytest.raises(ParserError) as exc:
        simple_parse({
            'type': 'object',
            'properties': {'field': {'type': 'xxxx'}},
            'additionalProperties': False,
        })
    assert exc.value.infile_path == '/definitions/type/properties/field/type'
    assert exc.value.msg == 'Unknown type "xxxx"'


def test_error_in_extra(simple_parse):
    with pytest.raises(ParserError) as exc:
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
        'vfull#/definitions/type': SchemaObject(
            properties={'field': Integer()},
            additionalProperties=Boolean(),
        ),
    }
