import pytest

from chaotic.front import parser as front_parser
from chaotic.front import types as front_types


def test_of_none(simple_parse):
    with pytest.raises(front_parser.ParserError) as exc:
        simple_parse({'allOf': []})
    assert exc.value.infile_path == '/definitions/type/allOf'
    assert exc.value.msg == 'Empty allOf'


# stupid, but valid
def test_1_empty(simple_parse):
    parsed = simple_parse({
        'allOf': [
            {'type': 'object', 'properties': {}, 'additionalProperties': False},
        ],
    })
    assert parsed.schemas == {
        'vfull#/definitions/type': front_types.AllOf(
            allOf=[front_types.SchemaObject(properties={}, additionalProperties=False)],
        ),
    }, f'Actual schema: {parsed.schemas}'


def test_2_empty(simple_parse):
    parsed = simple_parse({
        'allOf': [
            {
                'type': 'object',
                'properties': {},
                'additionalProperties': False,
            },
            {
                'type': 'object',
                'properties': {},
                'additionalProperties': False,
            },
        ],
    })
    assert parsed.schemas == {
        'vfull#/definitions/type': front_types.AllOf(
            allOf=[
                front_types.SchemaObject(properties={}, additionalProperties=False),
                front_types.SchemaObject(properties={}, additionalProperties=False),
            ],
        ),
    }, f'Actual schema: {parsed.schemas}'


def test_non_object(simple_parse):
    with pytest.raises(front_parser.ParserError) as exc:
        simple_parse({'allOf': [{'type': 'integer'}]})
    assert exc.value.infile_path == '/definitions/type/allOf/0'
    assert exc.value.msg == 'Non-object type in allOf: integer'


def test_additional_properties_mix_1(simple_parse):
    parsed = simple_parse({
        'allOf': [
            {
                'type': 'object',
                'properties': {},
                'additionalProperties': True,
            },
            {
                'type': 'object',
                'properties': {},
                'additionalProperties': False,
            },
        ],
    })
    assert parsed.schemas == {
        'vfull#/definitions/type': front_types.AllOf(
            allOf=[
                front_types.SchemaObject(properties={}, additionalProperties=True),
                front_types.SchemaObject(properties={}, additionalProperties=False),
            ],
        ),
    }, f'Actual schema: {parsed.schemas}'


def test_additional_properties_mix_2(simple_parse):
    parsed = simple_parse({
        'allOf': [
            {
                'type': 'object',
                'properties': {},
                'additionalProperties': False,
            },
            {
                'type': 'object',
                'properties': {},
                'additionalProperties': {
                    'type': 'object',
                    'additionalProperties': False,
                    'properties': {
                        'foo': {
                            'type': 'string',
                        },
                    },
                },
            },
        ],
    })

    assert parsed.schemas == {
        'vfull#/definitions/type': front_types.AllOf(
            allOf=[
                front_types.SchemaObject(properties={}, additionalProperties=False),
                front_types.SchemaObject(
                    properties={},
                    additionalProperties=front_types.SchemaObject(
                        properties={'foo': front_types.String()},
                        additionalProperties=False,
                    ),
                ),
            ],
        ),
    }, f'Actual schema: {parsed.schemas}'


def test_additional_properties_mix_3(simple_parse):
    parsed = simple_parse({
        'allOf': [
            {
                'type': 'object',
                'properties': {},
                'additionalProperties': True,
            },
            {
                'type': 'object',
                'properties': {},
                'additionalProperties': {
                    'type': 'object',
                    'properties': {
                        'foo': {
                            'type': 'string',
                        },
                    },
                },
            },
        ],
    })

    assert parsed.schemas == {
        'vfull#/definitions/type': front_types.AllOf(
            allOf=[
                front_types.SchemaObject(properties={}, additionalProperties=True),
                front_types.SchemaObject(
                    properties={},
                    additionalProperties=front_types.SchemaObject(
                        properties={'foo': front_types.String()},
                        additionalProperties=None,
                    ),
                ),
            ],
        ),
    }, f'Actual schema: {parsed.schemas}'
