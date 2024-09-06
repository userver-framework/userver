import pytest

from chaotic.front.parser import ParserConfig
from chaotic.front.parser import ParserError
from chaotic.front.parser import SchemaParser
from chaotic.front.types import Boolean
from chaotic.front.types import Integer
from chaotic.front.types import Number
from chaotic.front.types import OneOfWithDiscriminator
from chaotic.front.types import OneOfWithoutDiscriminator
from chaotic.front.types import Ref
from chaotic.front.types import SchemaObject
from chaotic.front.types import String


@pytest.fixture(name='parse_after_refs')
def _parse_after_refs():
    def func(input_: dict):
        config = ParserConfig(erase_prefix='')
        parser = SchemaParser(
            config=config, full_filepath='full', full_vfilepath='vfull',
        )
        parser.parse_schema(
            '/definitions/type1',
            {
                'type': 'object',
                'properties': {
                    'foo': {'type': 'string'},
                    'bar': {'type': 'number'},
                },
                'additionalProperties': False,
            },
        )
        parser.parse_schema(
            '/definitions/type2',
            {
                'type': 'object',
                'properties': {
                    'foo': {'type': 'string'},
                    'bar': {'type': 'boolean'},
                },
                'additionalProperties': False,
            },
        )
        parser.parse_schema('/definitions/type_int', {'type': 'integer'})
        parser.parse_schema(
            '/definitions/wrong_type',
            {
                'type': 'object',
                'properties': {'bar': {'type': 'integer'}},
                'additionalProperties': False,
            },
        )
        parser.parse_schema('/definitions/type', input_)
        return parser.parsed_schemas().schemas

    return func


REFS = {
    'vfull#/definitions/type1': SchemaObject(
        properties={'foo': String(), 'bar': Number()},
        additionalProperties=False,
    ),
    'vfull#/definitions/type2': SchemaObject(
        properties={'foo': String(), 'bar': Boolean()},
        additionalProperties=False,
    ),
    'vfull#/definitions/type_int': Integer(),
    'vfull#/definitions/wrong_type': SchemaObject(
        properties={'bar': Integer()}, additionalProperties=False,
    ),
}


def test_of_none(simple_parse):
    try:
        simple_parse({'oneOf': []})
        assert False
    except ParserError as exc:
        assert exc.infile_path == '/definitions/type/oneOf'
        assert exc.msg == 'Empty oneOf'


# stupid, but valid
def test_wo_discriminator_1(simple_parse):
    parsed = simple_parse({'oneOf': [{'type': 'integer'}]})
    assert parsed.schemas == {
        'vfull#/definitions/type': OneOfWithoutDiscriminator(
            oneOf=[Integer()],
        ),
    }


def test_wo_discriminator_2(simple_parse):
    parsed = simple_parse(
        {
            'oneOf': [
                {'type': 'integer'},
                {
                    'type': 'object',
                    'properties': {},
                    'additionalProperties': False,
                },
            ],
        },
    )
    assert parsed.schemas == {
        'vfull#/definitions/type': OneOfWithoutDiscriminator(
            oneOf=[
                Integer(),
                SchemaObject(properties={}, additionalProperties=False),
            ],
        ),
    }


def test_wd_no_ref_or_object(simple_parse):
    try:
        simple_parse(
            {
                'oneOf': [
                    {'type': 'integer'},
                    {
                        'type': 'object',
                        'properties': {},
                        'additionalProperties': False,
                    },
                ],
                'discriminator': {'propertyName': 'foo'},
            },
        )
        assert False
    except ParserError as exc:
        assert exc.infile_path == '/definitions/type/oneOf/0'
        assert exc.msg == 'Not a $ref in oneOf with discriminator'


def test_wd_wrong_property(simple_parse):
    try:
        simple_parse(
            {
                'oneOf': [
                    {
                        'type': 'object',
                        'properties': {},
                        'additionalProperties': False,
                    },
                ],
                'discriminator': {'propertyName': 'foo'},
            },
        )
        assert False
    except ParserError as exc:
        assert exc.infile_path == '/definitions/type/oneOf/0'
        assert exc.msg == 'Not a $ref in oneOf with discriminator'


def test_wd_wrong_property2(parse_after_refs):
    try:
        parse_after_refs(
            {
                'oneOf': [
                    {'$ref': '#/definitions/type1'},
                    {'$ref': '#/definitions/type2'},
                    {'$ref': '#/definitions/wrong_type'},
                ],
                'discriminator': {'propertyName': 'foo'},
            },
        )
        assert False
    except ParserError as exc:
        assert exc.infile_path == '/definitions/type/oneOf/2'
        assert exc.msg == 'No discriminator property "foo"'


def test_wd_wrong_type(parse_after_refs):
    try:
        parse_after_refs(
            {
                'oneOf': [{'$ref': '#/definitions/type_int'}],
                'discriminator': {'propertyName': 'foo'},
            },
        )
        assert False
    except ParserError as exc:
        assert exc.infile_path == '/definitions/type/oneOf/0'
        assert exc.msg == 'oneOf $ref to non-object'


def test_wd_ok(parse_after_refs):
    schema = parse_after_refs(
        {
            'oneOf': [
                {'$ref': '#/definitions/type1'},
                {'$ref': '#/definitions/type2'},
            ],
            'discriminator': {'propertyName': 'foo'},
        },
    )
    assert schema == {
        **REFS,
        'vfull#/definitions/type': OneOfWithDiscriminator(
            oneOf=[
                Ref(
                    ref='vfull#/definitions/type1',
                    indirect=False,
                    self_ref=False,
                ),
                Ref(
                    ref='vfull#/definitions/type2',
                    indirect=False,
                    self_ref=False,
                ),
            ],
            discriminator_property='foo',
            mapping=[['type1'], ['type2']],
        ),
    }


def test_wd_ok_with_mapping(parse_after_refs):
    schema = parse_after_refs(
        {
            'oneOf': [
                {'$ref': '#/definitions/type1'},
                {'$ref': '#/definitions/type2'},
            ],
            'discriminator': {
                'propertyName': 'foo',
                'mapping': {
                    't1': '#/definitions/type1',
                    't2': '#/definitions/type2',
                },
            },
        },
    )
    assert schema == {
        **REFS,
        'vfull#/definitions/type': OneOfWithDiscriminator(
            oneOf=[
                Ref(
                    ref='vfull#/definitions/type1',
                    indirect=False,
                    self_ref=False,
                ),
                Ref(
                    ref='vfull#/definitions/type2',
                    indirect=False,
                    self_ref=False,
                ),
            ],
            discriminator_property='foo',
            mapping=[['t1'], ['t2']],
        ),
    }


def test_wd_ok_with_mapping_missing_ref(parse_after_refs):
    try:
        parse_after_refs(
            {
                'oneOf': [
                    {'$ref': '#/definitions/type1'},
                    {'$ref': '#/definitions/type2'},
                ],
                'discriminator': {
                    'propertyName': 'foo',
                    'mapping': {'t2': '#/definitions/type2'},
                },
            },
        )
        assert False
    except ParserError as exc:
        assert exc.infile_path == '/definitions/type/discriminator/mapping'
        assert exc.msg == 'Missing $ref in mapping: vfull#/definitions/type1'


def test_wd_ok_with_mapping_invalid_ref(parse_after_refs):
    try:
        parse_after_refs(
            {
                'oneOf': [
                    {'$ref': '#/definitions/type1'},
                    {'$ref': '#/definitions/type2'},
                ],
                'discriminator': {
                    'propertyName': 'foo',
                    'mapping': {
                        't1': '#/definitions/wrong',
                        't3': '#/definitions/type1',
                        't2': '#/definitions/type2',
                    },
                },
            },
        )
        assert False
    except ParserError as exc:
        assert exc.infile_path == '/definitions/type/discriminator/mapping'
        assert exc.msg == (
            '$ref(s) outside of oneOf: [\'vfull#/definitions/wrong\']'
        )


def test_wd_invalidtype_mapping_value(parse_after_refs):
    try:
        parse_after_refs(
            {
                'oneOf': [
                    {'$ref': '#/definitions/type1'},
                    {'$ref': '#/definitions/type2'},
                ],
                'discriminator': {
                    'propertyName': 'foo',
                    'mapping': {'t1': 1, 't2': '#/definitions/type2'},
                },
            },
        )
        assert False
    except ParserError as exc:
        assert exc.infile_path == '/definitions/type/discriminator/mapping/t1'
        assert exc.msg == 'Not a string in mapping'


def test_wd_extra_field(simple_parse):
    try:
        simple_parse(
            {
                'oneOf': [
                    {
                        'type': 'object',
                        'properties': {},
                        'additionalProperties': False,
                    },
                ],
                'discriminator': {'foo': 1, 'propertyName': 'foo'},
            },
        )
        assert False
    except ParserError as exc:
        assert exc.infile_path == '/definitions/type/discriminator/foo'
        assert exc.msg == (
            'Unknown field: "foo", known fields: ["mapping", "propertyName"]'
        )
